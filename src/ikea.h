ikea_t* ikea_open( context_t * C, elem_t * name );
success_t ikea_append(ikea_t* ikea, unsigned char *data, size_t data_len);
success_t ikea_flush(ikea_t* ikea);
success_t ikea_close(ikea_t* ikea);

