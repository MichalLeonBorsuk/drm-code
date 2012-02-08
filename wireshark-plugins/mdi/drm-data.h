#ifndef _DRM_DATA_H
#define _DRM_DATA_H

typedef struct _packet_stream_t {
  gboolean data_unit_indicator;
  guint8 application_domain;
  guint8 *application_data;
  gboolean valid;
} packet_stream_t;

typedef struct _stream_t {
  guint16 part_a_len, part_b_len;
  gboolean audio_not_data;
  guint8 audio_coding;
  gboolean sbr;
  guint8 audio_mode;
  guint8 sampling_rate_index;
  gboolean text_active;
  gboolean enhancement_flag;
  guint8 coder_field;
  gboolean packet_mode_indicator;
  guint16 packet_length;
  packet_stream_t data_services[4];
  gboolean valid;
} stream_t;

typedef struct _drm_data_t {
  guint16 maj, min;
  guint8 robm, prot_a, prot_b, streams, stream;
  nstime_t tist;
  gint protection_a, protection_b;
  gint VSPP;
  gint spectral_occupancy;
  gint interleaver_depth;
  gint msc_mode;
  gint sdc_mode;
  stream_t stream_data[4];
  guint sdc_bytes;
} drm_data_t;

#endif
