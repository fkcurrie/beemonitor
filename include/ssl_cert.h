#ifndef SSL_CERT_H
#define SSL_CERT_H

const uint8_t CERT_START[] asm("_binary_data_cert_pem_start");
const uint8_t CERT_END[]   asm("_binary_data_cert_pem_end");
const uint8_t KEY_START[]  asm("_binary_data_key_pem_start");
const uint8_t KEY_END[]    asm("_binary_data_key_pem_end");

#endif // SSL_CERT_H
