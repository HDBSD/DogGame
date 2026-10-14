#ifndef XBOX_INPUT_METHOD_H
#define XBOX_INPUT_METHOD_H
/* Player one's last intentional input. Idle/drifting controllers and
 * synthesized startup button pulses do not change prompt presentation. */
int xbox_InputUsesKeyboard(void);
#endif
