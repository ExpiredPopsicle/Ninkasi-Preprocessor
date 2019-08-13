#ifndef NK_PPMEM_H
#define NK_PPMEM_H

void *nkppDefaultMallocWrapper(void *userData, nkuint32_t size);
void nkppDefaultFreeWrapper(void *userData, void *ptr);

#endif // NK_PPMEM_H

