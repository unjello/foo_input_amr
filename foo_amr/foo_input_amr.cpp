/**
 * foo_input_amr - amr-nb decoder for foobar2000
*/

/* include foobar sdk */
#include "../foo_sdk/foobar2000/SDK/foobar2000.h"
/* include 3gpp amr-nb decoder */
extern "C" { 
	#include "../3gpp/interf_dec.h" 
}
/* enable logging only in debug mode */
#ifdef _DEBUG
	#define SPDLOG_DEBUG_ON
	#define SPDLOG_TRACE_ON
#endif
#include "spdlog\spdlog.h"

enum {
	/* AMR files have 8bit samples */
	amr_bits_per_sample = 8,
	/* AMR files are mono */
	amr_channels = 1,
	/* AMR files are ~8000khz */
	amr_sample_rate = 8000,
	/* AMR frame is 20ms long */
	amr_frame_sample_length = 20,

	/**
	 * helper contants derived from above
	 * @{
	 */
	amr_audio_frame_size = amr_frame_sample_length * amr_bits_per_sample,
	amr_bytes_per_sample = amr_bits_per_sample / 8,
	amr_total_sample_width = amr_bytes_per_sample * amr_channels,
	/** 
	 * @} 
	 */
};

/**
 * AMR decoder's plugin class. No inheritance. Foobar uses advanced template magic to
 * call functions. Plugin API was the main change since foobar 0.9.5.5
 *
 * @author  Andrzej Lichnerowicz
 * @version 1.1.1
 * @since   1.1.0
 */
class input_amr : public input_stubs {
protected:
	/**
	 * Checks if loaded file is valid AMR audio record. The check is done based on magic string in header.
	 * 
	 * @param p_abort		abort callback provided by foobar.
	 * @return				<code>true</code> if the file looks like AMR
	 *						<code>false</code> in any other case, including errors
	 * @see					m_magic
	 * @since				1.0.0
	 */
	bool is_amr(abort_callback & p_abort) {
		/* static buffer to store magic string */
		static char head[5];
		
		/* read the magic string from the file */
		int read = m_file->read(head, 5, p_abort);
		/* seek back to the begining */
		m_file->seek(0, p_abort);
		
		/* if read was successful and head matches amr magic string */
		return read != 5 || memcmp(head,m_magic,5) ? false : true;
	}

	/**
	 * Retrives number of audio frames stored in AMR file. Each frame stores 20ms of audio. The total
	 * number is not stored in the file. Frames start right after magic string. Each frame can be of 
	 * different length, because each can be encoded at different rate (hence Adaptive Multirate).
	 * To retrive total number of frames, we must scan through them in a whole file.
	 * 
	 * @param p_abort		abort callback provided by foobar.
	 * @return				total nuber of 20ms frames
	 * @see					m_start
	 * @see					m_block_size
	 * @since				1.0.0
	 */
	unsigned decode_length(abort_callback & p_abort) {
		char id;
		unsigned size, frames = 0;

		/* seek at the begining of the first frame */
		m_file->seek(m_start,p_abort);
		/* while we can stil read, go through frames */
		while(m_file->read(&id,sizeof(char),p_abort)) {
			/* first byte is rate mode. each rate mode has frame of given length. look it up. */
			size = m_block_size[(id >> 3) & 0x000F];
			/* we're not interested. just skip this frame, and go to another. */
			m_file->seek_ex(size, file::seek_from_current, p_abort);
			/* increase number of frames */
			++frames;
		}
		/* go at the begining */
		m_file->seek(0,p_abort);

		/* return number of frames found */
		return frames;
	}


public:
	static const char * g_get_name() { return "foo_input_amr amr decoder"; }

	static const GUID g_get_guid() {
		static const GUID foo_input_amr_GUID =
		{ 0x9160f16c, 0x62ce, 0x487c,{ 0xa3, 0x7a, 0xaf, 0x53, 0x73, 0x37, 0xf3, 0xe2 } };

		return foo_input_amr_GUID;
	}

	/**
	 * API function called by foobar to open a file. It's called on get info or playback start. It's safest place
	 * to place total_frame_count extraction code.
	 * 
	 * @param p_filehint	file object, may be null untill opened.
	 * @param p_path		path to file
	 * @param p_reason		reason why the file was opened
	 * @param p_abort		abort callback
	 * @since				1.0.0
	 */
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		/* write access is called for retagging purposes. we do not support that, so throw an error */
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();

		/* store file object */
		m_file = p_filehint;

		/* if file is null, open it - handled by the helper */
		input_open_file_helper(m_file,p_path,p_reason,p_abort);

		/* ensure input stream can seek */
		m_file->ensure_seekable();

		/* store total frames count of amr file */
		m_frames = decode_length(p_abort);
	}


	/**
	 * API function called by foobar to get information of properties dialog. AMR is easy, 
	 * since most of the info is pretty constant.
	 * 
	 * @param p_info		object to store the info in
	 * @param p_abort		abort callback
	 * @since				1.1.0
	 */
	void get_info(file_info & p_info,abort_callback & p_abort) {
		p_info.set_length((double)m_frames*amr_audio_frame_size/amr_sample_rate);
		p_info.info_set_bitrate((amr_bits_per_sample * amr_channels * amr_sample_rate + 500 /* rounding for bps to kbps*/ ) / 1000 /* bps to kbps */);
		p_info.info_set_int("samplerate",amr_sample_rate);
		p_info.info_set_int("channels",amr_channels);
		p_info.info_set_int("bitspersample",amr_bits_per_sample);
		p_info.info_set("encoding","Adaptive Multirate");		
	}

	/**
	 * API function called by foobar to initialize decoder. We relay that init to start 3gpp's AMR decoder
	 * and reset internal counters.
	 * 
	 * @param p_flags		unused
	 * @param p_abort		abort callback
	 * @since				1.0.0
	 */
	void decode_initialize(unsigned p_flags,abort_callback & p_abort) {
		/* equivalent to seek to zero, except it also works on nonseekable streams */
		m_file->reopen(p_abort);

		/* initialize 3gpp's amr decoder */
		m_state = reinterpret_cast<int*>(Decoder_Interface_init());
		/* seek to the first frame */
		m_file->seek(m_start, p_abort);

		/* reserve buffer for decoded audio */
		m_data.set_size(amr_audio_frame_size);
		/* we start at first frame */
		m_frame = 0;
	}

	/**
	 * API function called by foobar to get next chunk of audio. 
	 * 
	 * @param p_chunk		buffer in which we store decoded audio
	 * @param p_abort		abort callback
	 * @see					m_block_size
	 * @since				1.1.0
	 */
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		/* return false if we've reached total frames count */
		if(m_frame>=m_frames) return 0;
		
		/* read mode type */
		m_file->read(m_buffer, sizeof(char), p_abort);
		/* get frame size for this frame's amr mode */
		unsigned read_size = m_block_size[(m_buffer[0]>>3)&0x000F];

		/* read the buffer */
		m_file->read(&m_buffer[1], sizeof(char)*read_size, p_abort);
		/* decode next portion of audio */
		Decoder_Interface_Decode(m_state, m_buffer, m_data.get_ptr(), 0);

		/* feed foobar with what we got */
		p_chunk.set_data_fixedpoint(m_data.get_ptr(),
									amr_audio_frame_size*2, /* foobar seems to convert what we gave to 16bps */
									amr_sample_rate,
									amr_channels,
									16, /* foobar seems to convert what we gave to 16bps */
									audio_chunk::g_guess_channel_config(amr_channels)
		);
		/* "move" to the next frame */
		++m_frame;

		/* we're ready for more processing */
		return 1;
	}

	/**
	 * API function called by foobar when user touches seeking bar. 
	 * 
	 * @param p_seconds		position on seeking bar that user have choosen
	 * @param p_abort		abort callback
	 * @see					m_block_size
	 * @since				1.1.0
	 */
	void decode_seek(double p_seconds, abort_callback & p_abort) {
		char id;
		unsigned size;

		/* throw exceptions if someone called decode_seek() despite of our input having reported itself as nonseekable. */
		m_file->ensure_seekable();
		/* calculate target frame from given time */
		t_filesize target = audio_math::time_to_samples(p_seconds, amr_sample_rate) / amr_audio_frame_size;

		/**
		 * since there is no way to tell the position of given frame in the file stream 
		 * we need to go through all of them untill we find the frame we're looking for.
		 * @{
		 */
		m_file->seek(m_start,p_abort);
		m_frame = 0;
		while(m_file->read(&id, sizeof(char), p_abort) && m_frame < target) {
			size = m_block_size[(id >> 3) & 0x000F];
			m_file->seek_ex(size, file::seek_from_current, p_abort);
			++m_frame;
		}
		/**
		 * @}
		 */
	}

	/* we're able to seek */
	bool decode_can_seek() {return true; }
	/* no fancy stuff */
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { return false; }
	/* no fancy stuff */
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) { return false; }
	/* simple relay */
	void decode_on_idle(abort_callback & p_abort) {m_file->on_idle(p_abort);}
	/* simple relay */
	t_filestats get_file_stats(abort_callback & p_abort) {return m_file->get_stats(p_abort);}
	/* no fancy stuff */
	void retag(const file_info & p_info,abort_callback & p_abort) {throw exception_io_unsupported_format();}
	
	/* identify amr by content type */
	static bool g_is_our_content_type(const char * p_content_type) {
		ensure_log_exists();
		bool ret = stricmp_utf8(p_content_type,"audio/amr") == 0 || stricmp_utf8(p_content_type,"audio/x-amr") == 0; 
		SPDLOG_TRACE(log, "Identify content-type '{}': {}", p_content_type, ret?"true":"false");
		return ret;
	}
	/* identify amr by file extension */
	static bool g_is_our_path(const char * p_path,const char * p_extension) {
		ensure_log_exists();
		bool ret = stricmp_utf8(p_extension,"amr") == 0;
		SPDLOG_TRACE(log, "Identify extension '{}': {}", p_extension, ret?"true":"false");
		return ret;
	}

public:
	service_ptr_t<file> m_file;
	pfc::array_t<t_int16> m_data;
	int* m_state;
	static const char* m_magic;
	static const unsigned m_start;
	static short m_block_size[16];
	unsigned char m_buffer[32];
	unsigned m_frames;
	unsigned m_frame;

private:
	static void ensure_log_exists() {
#ifdef _DEBUG
		if (!log) {
			pfc::string8 tempPath;
			if (!uGetTempPath(tempPath)) uBugCheck();
			tempPath.add_filename("foo_input_amr.txt");
			log = spdlog::basic_logger_mt("amr", tempPath.c_str());
			log->set_level(spdlog::level::trace);
		}
#endif
	}
#ifdef _DEBUG
	static std::shared_ptr<spdlog::logger> log;
#endif
};

/**
 * there are 8 varying levels of compression:
 * mode		bitrates
 * 0		amr 4.75
 * 1		amr 5.15
 * 2		amr 5.9
 * 3		amr 6.7
 * 4		amr 7.4
 * 5		amr 7.95
 * 6		amr 10.2
 * 7		amr 12.2
 * first byte of the frame specifies CMR (codec mode request), values 0-7 are valid for AMR.
 * each mode have different frame size. this table reflects that fact.
*/
short input_amr::m_block_size[] =  { 12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0 };
/* each AMR-NB file consists of following 6-byte header */
const char* input_amr::m_magic = "#!AMR\x0a";
/* AMR frames start at 7-th byte */
const unsigned input_amr::m_start = 7;
#ifdef _DEBUG
std::shared_ptr<spdlog::logger> input_amr::log;
#endif

/**
 * plugin factory 
 * @{
 */
static input_singletrack_factory_t<input_amr> g_input_amr_factory;
DECLARE_COMPONENT_VERSION("AMR input","1.1.2","https://github.com/unjello/foo_input_amr/; 2003-2018: Andrzej Lichnerowicz, Quang Nguyen\nPowered GSM AMR-NB speech codec\n(c) 2001, 3gpp");
DECLARE_FILE_TYPE("Adaptive Multirate files","*.AMR");
/**
 * @}
 */