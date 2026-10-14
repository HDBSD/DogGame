#ifndef TITLE_SAVES_H
#define TITLE_SAVES_H
/* Create per-user storage and install the guest-path adapter. Writes the
 * UTF-8 runtime storage path on success; returns zero on initialization failure. */
int title_saves_init(const char *game_directory, char *storage, unsigned capacity);
#endif
