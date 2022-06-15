
void sd_init();
void sd_mount();
void readblock(int block_idx, void *buf);
void writeblock(int block_idx, void *buf);