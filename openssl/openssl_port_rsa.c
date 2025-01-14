// This file contains RSA portability wrappers.
// +build linux
// +build !android
// +build !no_openssl
// +build !cmd_go_bootstrap
// +build !msan

#include "goopenssl.h"

// Only in BoringSSL.
GO_RSA *_goboringcrypto_RSA_generate_key_fips(int bits) {
  GO_EVP_PKEY_CTX *ctx = NULL;
  GO_EVP_PKEY *pkey = NULL;
  GO_BIGNUM *e = NULL;
  GO_RSA *ret = NULL;

  ctx = _goboringcrypto_EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
  if (!ctx)
    return NULL;

  if (_goboringcrypto_EVP_PKEY_keygen_init(ctx) <= 0)
    goto err;

  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits) <= 0)
    goto err;

  // BoringSSL's RSA_generate_key_fips hard-codes e to 65537.
  e = _goboringcrypto_BN_new();
  if (!e)
    goto err;

  if (_goboringcrypto_BN_set_word(e, RSA_F4) <= 0)
    goto err;

  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx, e) <= 0)
    goto err;

  if (_goboringcrypto_EVP_PKEY_keygen(ctx, &pkey) <= 0)
    goto err;

  ret = _goboringcrypto_EVP_PKEY_get1_RSA(pkey);

err:
  _goboringcrypto_BN_free(e);
  _goboringcrypto_EVP_PKEY_free(pkey);
  _goboringcrypto_EVP_PKEY_CTX_free(ctx);
  return ret;
}

int _goboringcrypto_RSA_digest_and_sign_pss_mgf1(
    GO_RSA *rsa, size_t *out_len, uint8_t *out, size_t max_out,
    const uint8_t *in, size_t in_len, EVP_MD *md, const EVP_MD *mgf1_md,
    int salt_len) {
  EVP_PKEY_CTX *ctx;
  unsigned int siglen;

  int ret = 0;
  EVP_PKEY *key = _goboringcrypto_EVP_PKEY_new();
  if (!key) {
    goto err;
  }
  if (!_goboringcrypto_EVP_PKEY_set1_RSA(key, rsa))
    goto err;
  ctx = _goboringcrypto_EVP_PKEY_CTX_new(key, NULL /* no engine */);
  if (!ctx)
    goto err;

  EVP_MD_CTX *mdctx = NULL;
  if (!(mdctx = _goboringcrypto_EVP_MD_CTX_create()))
    goto err;

  if (1 != _goboringcrypto_EVP_DigestSignInit(mdctx, &ctx, md, NULL, key))
    goto err;

  if (_goboringcrypto_EVP_PKEY_sign_init(ctx) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_signature_md(ctx, md) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_padding(ctx,
                                                   RSA_PKCS1_PSS_PADDING) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_pss_saltlen(ctx, salt_len) <= 0)
    goto err;
  if (mgf1_md)
    if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, mgf1_md) <= 0)
      goto err;
  if (1 != _goboringcrypto_EVP_DigestUpdate(mdctx, in, in_len))
    goto err;

  /* Obtain the signature length */
  if (1 != _goboringcrypto_EVP_DigestSignFinal(mdctx, NULL, out_len))
    goto err;
  /* Obtain the signature */
  if (1 != _goboringcrypto_EVP_DigestSignFinal(mdctx, out, out_len))
    goto err;

  ret = 1;

err:
  if (mdctx)
    _goboringcrypto_EVP_MD_CTX_free(mdctx);
  if (ctx)
    _goboringcrypto_EVP_PKEY_CTX_free(ctx);
  if (key)
    _goboringcrypto_EVP_PKEY_free(key);

  return ret;
}

int _goboringcrypto_RSA_sign_pss_mgf1(GO_RSA *rsa, unsigned int *out_len,
                                      uint8_t *out, unsigned int max_out,
                                      const uint8_t *in, unsigned int in_len,
                                      EVP_MD *md, const EVP_MD *mgf1_md,
                                      int salt_len) {
  EVP_PKEY_CTX *ctx;
  EVP_PKEY *pkey;
  size_t siglen;

  int ret = 0;
  pkey = _goboringcrypto_EVP_PKEY_new();
  if (!pkey)
    goto err;

  if (_goboringcrypto_EVP_PKEY_set1_RSA(pkey, rsa) <= 0)
    goto err;

  ctx = _goboringcrypto_EVP_PKEY_CTX_new(pkey, NULL /* no engine */);
  if (!ctx)
    goto err;

  if (_goboringcrypto_EVP_PKEY_sign_init(ctx) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_signature_md(ctx, md) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_padding(ctx,
                                                   RSA_PKCS1_PSS_PADDING) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_pss_saltlen(ctx, salt_len) <= 0)
    goto err;
  if (mgf1_md)
    if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, mgf1_md) <= 0)
      goto err;
  /* Determine buffer length */
  if (_goboringcrypto_EVP_PKEY_sign(ctx, NULL, &siglen, in, in_len) <= 0)
    goto err;

  if (max_out < siglen)
    goto err;

  if (_goboringcrypto_EVP_PKEY_sign(ctx, out, &siglen, in, in_len) <= 0)
    goto err;

  *out_len = siglen;
  ret = 1;

err:
  if (ctx)
    _goboringcrypto_EVP_PKEY_CTX_free(ctx);
  if (pkey)
    _goboringcrypto_EVP_PKEY_free(pkey);

  return ret;
}

int _goboringcrypto_RSA_verify_pss_mgf1(RSA *rsa, const uint8_t *msg,
                                        unsigned int msg_len, EVP_MD *md,
                                        const EVP_MD *mgf1_md, int salt_len,
                                        const uint8_t *sig,
                                        unsigned int sig_len) {
  EVP_PKEY_CTX *ctx;
  EVP_PKEY *pkey;

  int ret = 0;

  pkey = _goboringcrypto_EVP_PKEY_new();
  if (!pkey)
    goto err;

  if (_goboringcrypto_EVP_PKEY_set1_RSA(pkey, rsa) <= 0)
    goto err;

  ctx = _goboringcrypto_EVP_PKEY_CTX_new(pkey, NULL /* no engine */);
  if (!ctx)
    goto err;

  if (_goboringcrypto_EVP_PKEY_verify_init(ctx) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_padding(ctx,
                                                   RSA_PKCS1_PSS_PADDING) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_pss_saltlen(ctx, salt_len) <= 0)
    goto err;
  if (_goboringcrypto_EVP_PKEY_CTX_set_signature_md(ctx, md) <= 0)
    goto err;
  if (mgf1_md)
    if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, mgf1_md) <= 0)
      goto err;
  if (_goboringcrypto_EVP_PKEY_verify(ctx, sig, sig_len, msg, msg_len) <= 0)
    goto err;

  ret = 1;

err:
  if (ctx)
    _goboringcrypto_EVP_PKEY_CTX_free(ctx);
  if (pkey)
    _goboringcrypto_EVP_PKEY_free(pkey);

  return ret;
}

int _goboringcrypto_RSA_sign(EVP_MD *md, const uint8_t *msg,
			     unsigned int msgLen, uint8_t *sig,
			     size_t *slen, RSA *rsa) {
  int result;
  EVP_PKEY *key = _goboringcrypto_EVP_PKEY_new();
  if (!key) {
    return 0;
  }
  if (!_goboringcrypto_EVP_PKEY_set1_RSA(key, rsa)) {
    result = 0;
    goto err;
  }
  result = _goboringcrypto_EVP_sign(md, NULL, msg, msgLen, sig, slen, key);
err:
  _goboringcrypto_EVP_PKEY_free(key);
  return result;
}

int _goboringcrypto_RSA_verify(EVP_MD *md, const uint8_t *msg,
			       unsigned int msgLen, const uint8_t *sig,
			       unsigned int slen, GO_RSA *rsa) {
  int result;
  EVP_PKEY *key = _goboringcrypto_EVP_PKEY_new();
  if (!key) {
    return 0;
  }
  if (!_goboringcrypto_EVP_PKEY_set1_RSA(key, rsa)) {
    result = 0;
    goto err;
  }
  result = _goboringcrypto_EVP_verify(md, NULL, msg, msgLen, sig, slen, key);
err:
  _goboringcrypto_EVP_PKEY_free(key);
  return result;
}

int _goboringcrypto_RSA_sign_raw(EVP_MD *md, const uint8_t *msg,
				 size_t msgLen, uint8_t *sig, size_t *slen,
				 GO_RSA *rsa_key) {
  int ret = 0;
  GO_EVP_PKEY_CTX *ctx = NULL;
  GO_EVP_PKEY *pkey = NULL;

  pkey = _goboringcrypto_EVP_PKEY_new();
  if (!pkey)
    goto err;

  if (_goboringcrypto_EVP_PKEY_set1_RSA(pkey, rsa_key) != 1)
    goto err;

  ctx = _goboringcrypto_EVP_PKEY_CTX_new(pkey, NULL);
  if (!ctx)
    goto err;

  if (_goboringcrypto_EVP_PKEY_sign_init(ctx) != 1)
    goto err;

  if (md && _goboringcrypto_EVP_PKEY_CTX_set_signature_md(ctx, md) != 1)
    goto err;

  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) != 1)
    goto err;

  if (_goboringcrypto_EVP_PKEY_sign(ctx, sig, slen, msg, msgLen) != 1)
    goto err;

  /* Success */
  ret = 1;

err:
  _goboringcrypto_EVP_PKEY_CTX_free(ctx);
  _goboringcrypto_EVP_PKEY_free(pkey);

  return ret;
}

int _goboringcrypto_RSA_verify_raw(EVP_MD *md,
				   const uint8_t *msg, size_t msgLen,
				   const uint8_t *sig, unsigned int slen,
				   GO_RSA *rsa_key) {
  int ret = 0;
  GO_EVP_PKEY_CTX *ctx = NULL;
  GO_EVP_PKEY *pkey = NULL;

  pkey = _goboringcrypto_EVP_PKEY_new();
  if (!pkey)
    goto err;

  if (_goboringcrypto_EVP_PKEY_set1_RSA(pkey, rsa_key) != 1)
    goto err;

  ctx = _goboringcrypto_EVP_PKEY_CTX_new(pkey, NULL);
  if (!ctx)
    goto err;

  if (_goboringcrypto_EVP_PKEY_verify_init(ctx) != 1)
    goto err;

  if (md && _goboringcrypto_EVP_PKEY_CTX_set_signature_md(ctx, md) != 1)
    goto err;

  if (_goboringcrypto_EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) != 1)
    goto err;

  if (_goboringcrypto_EVP_PKEY_verify(ctx, sig, slen, msg, msgLen) != 1)
    goto err;

  /* Success */
  ret = 1;

err:
  _goboringcrypto_EVP_PKEY_CTX_free(ctx);
  _goboringcrypto_EVP_PKEY_free(pkey);

  return ret;
}
