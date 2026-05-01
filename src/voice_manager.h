#ifndef VOICE_MANAGER_H
#define VOICE_MANAGER_H

void voice_init();
void voice_update();

// Callback for detected commands
typedef void (*voice_command_cb_t)(int command_id);
void voice_set_command_callback(voice_command_cb_t cb);

#endif
