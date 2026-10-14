#ifndef TITLE_GUEST_DIAGNOSTICS_H
#define TITLE_GUEST_DIAGNOSTICS_H
/* Indirect-call diagnostics retain the original routine and inspect its guest records. */
void traced_helmet_contact(void);
void traced_game_event(void);
void traced_io_poll(void);
void traced_texture_convert(void);
void traced_asset_read(void);
void traced_resource_read(void);
/* Observation-only worker: volatile reads are not a coherent snapshot. */
void title_guest_log_observed_state(void);
#endif
