// Private interface to library
#ifndef XMP_PRIVATE_H
#define XMP_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

extern const struct format_loader xm_loader;
extern const struct format_loader mod_loader;
extern const struct format_loader flt_loader;
extern const struct format_loader st_loader;
extern const struct format_loader it_loader;
extern const struct format_loader s3m_loader;
extern const struct format_loader stm_loader;
extern const struct format_loader stx_loader;
extern const struct format_loader mtm_loader;
extern const struct format_loader ice_loader;
extern const struct format_loader imf_loader;
extern const struct format_loader ptm_loader;
extern const struct format_loader mdl_loader;
extern const struct format_loader ult_loader;
extern const struct format_loader liq_loader;
extern const struct format_loader no_loader;
extern const struct format_loader masi_loader;
extern const struct format_loader gal5_loader;
extern const struct format_loader gal4_loader;
extern const struct format_loader psm_loader;
extern const struct format_loader amf_loader;
extern const struct format_loader asylum_loader;
extern const struct format_loader gdm_loader;
extern const struct format_loader mmd1_loader;
extern const struct format_loader mmd3_loader;
extern const struct format_loader med2_loader;
extern const struct format_loader med3_loader;
extern const struct format_loader med4_loader;
extern const struct format_loader dmf_loader;
extern const struct format_loader rtm_loader;
extern const struct format_loader pt3_loader;
extern const struct format_loader tcb_loader;
extern const struct format_loader dt_loader;
extern const struct format_loader gtk_loader;
extern const struct format_loader dtt_loader;
extern const struct format_loader mgt_loader;
extern const struct format_loader arch_loader;
extern const struct format_loader sym_loader;
extern const struct format_loader digi_loader;
extern const struct format_loader dbm_loader;
extern const struct format_loader emod_loader;
extern const struct format_loader okt_loader;
extern const struct format_loader sfx_loader;
extern const struct format_loader far_loader;
extern const struct format_loader umx_loader;
extern const struct format_loader stim_loader;
extern const struct format_loader coco_loader;
extern const struct format_loader mtp_loader;
extern const struct format_loader ims_loader;
extern const struct format_loader ssn_loader;
extern const struct format_loader fnk_loader;
extern const struct format_loader mfp_loader;
/* extern const struct format_loader alm_loader; */
extern const struct format_loader polly_loader;
extern const struct format_loader pw_loader;
extern const struct format_loader hmn_loader;

int xmp_load_typed_module_from_memory(xmp_context opaque, void *mem, long size, const struct format_loader* loader);
const char* xmp_get_loader_name(const struct format_loader* loader);

#ifdef __cplusplus
}
#endif

#endif
