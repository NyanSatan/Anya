#ifndef ANYA_ASTRIS_H
#define ANYA_ASTRIS_H

int astris_api_init(const char *astris_root);

int astris_probe_pick(char **probe);
int astris_soc_identify(const char *probe, char **soc, char **rev);

extern long astris_version;

#endif
