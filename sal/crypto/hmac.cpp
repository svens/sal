#include <sal/crypto/hmac.hpp>


__sal_begin


namespace crypto {


#if __sal_os_darwin

// md5 {{{1

template <>
hmac_t<md5>::hmac_t (const void *key, size_t size)
{
  ::CCHmacInit(&ctx_[0], kCCHmacAlgMD5, key, size);
  ctx_[1] = ctx_[0];
}

template <>
void hmac_t<md5>::update (const void *data, size_t size)
{
  ::CCHmacUpdate(&ctx_[0], data, size);
}

template <>
void hmac_t<md5>::finish (void *result, size_t)
{
  ::CCHmacFinal(&ctx_[0], result);
  ctx_[0] = ctx_[1];
}

template <>
void hmac_t<md5>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  ::CCHmac(kCCHmacAlgMD5, key, key_size, data, data_size, result);
}

// sha1 {{{1

template <>
hmac_t<sha1>::hmac_t (const void *key, size_t size)
{
  ::CCHmacInit(&ctx_[0], kCCHmacAlgSHA1, key, size);
  ctx_[1] = ctx_[0];
}

template <>
void hmac_t<sha1>::update (const void *data, size_t size)
{
  ::CCHmacUpdate(&ctx_[0], data, size);
}

template <>
void hmac_t<sha1>::finish (void *result, size_t)
{
  ::CCHmacFinal(&ctx_[0], result);
  ctx_[0] = ctx_[1];
}

template <>
void hmac_t<sha1>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  ::CCHmac(kCCHmacAlgSHA1, key, key_size, data, data_size, result);
}

// sha256 {{{1

template <>
hmac_t<sha256>::hmac_t (const void *key, size_t size)
{
  ::CCHmacInit(&ctx_[0], kCCHmacAlgSHA256, key, size);
  ctx_[1] = ctx_[0];
}

template <>
void hmac_t<sha256>::update (const void *data, size_t size)
{
  ::CCHmacUpdate(&ctx_[0], data, size);
}

template <>
void hmac_t<sha256>::finish (void *result, size_t)
{
  ::CCHmacFinal(&ctx_[0], result);
  ctx_[0] = ctx_[1];
}

template <>
void hmac_t<sha256>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  ::CCHmac(kCCHmacAlgSHA256, key, key_size, data, data_size, result);
}

// sha384 {{{1

template <>
hmac_t<sha384>::hmac_t (const void *key, size_t size)
{
  ::CCHmacInit(&ctx_[0], kCCHmacAlgSHA384, key, size);
  ctx_[1] = ctx_[0];
}

template <>
void hmac_t<sha384>::update (const void *data, size_t size)
{
  ::CCHmacUpdate(&ctx_[0], data, size);
}

template <>
void hmac_t<sha384>::finish (void *result, size_t)
{
  ::CCHmacFinal(&ctx_[0], result);
  ctx_[0] = ctx_[1];
}

template <>
void hmac_t<sha384>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  ::CCHmac(kCCHmacAlgSHA384, key, key_size, data, data_size, result);
}

// sha512 {{{1

template <>
hmac_t<sha512>::hmac_t (const void *key, size_t size)
{
  ::CCHmacInit(&ctx_[0], kCCHmacAlgSHA512, key, size);
  ctx_[1] = ctx_[0];
}

template <>
void hmac_t<sha512>::update (const void *data, size_t size)
{
  ::CCHmacUpdate(&ctx_[0], data, size);
}

template <>
void hmac_t<sha512>::finish (void *result, size_t)
{
  ::CCHmacFinal(&ctx_[0], result);
  ctx_[0] = ctx_[1];
}

template <>
void hmac_t<sha512>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  ::CCHmac(kCCHmacAlgSHA512, key, key_size, data, data_size, result);
}

// }}}1

#elif __sal_os_linux

namespace __bits { //{{{1

namespace {

inline void fix_key (const void *&key, size_t &size) noexcept
{
  if (!key)
  {
    key = "";
    size = 0U;
  }
}

} // namespace

hmac_ctx_t::hmac_ctx_t (const EVP_MD *evp, const void *key, size_t size) noexcept
  : ctx(std::make_unique<HMAC_CTX>())
{
  fix_key(key, size);
  ::HMAC_CTX_init(ctx.get());
  ::HMAC_Init_ex(ctx.get(), key, size, evp, nullptr);
}

hmac_ctx_t::~hmac_ctx_t () noexcept
{
  if (ctx)
  {
    ::HMAC_CTX_cleanup(ctx.get());
  }
}

hmac_ctx_t::hmac_ctx_t (const hmac_ctx_t &that)
{
  if (that.ctx)
  {
    ctx = std::make_unique<HMAC_CTX>();
    ::HMAC_CTX_copy(ctx.get(), that.ctx.get());
  }
}

void hmac_ctx_t::update (const void *data, size_t size)
{
  ::HMAC_Update(ctx.get(), static_cast<const uint8_t *>(data), size);
}

void hmac_ctx_t::finish (void *result)
{
  ::HMAC_Final(ctx.get(), static_cast<uint8_t *>(result), nullptr);
  ::HMAC_Init_ex(ctx.get(), nullptr, 0U, nullptr, nullptr);
}

void hmac_ctx_t::one_shot (const EVP_MD *evp,
  const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result)
{
  __bits::fix_key(key, key_size);
  ::HMAC(evp,
    static_cast<const uint8_t *>(key), key_size,
    static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(result), nullptr
  );
}

} // namespace __bits

// md5 {{{1

template <>
hmac_t<md5>::hmac_t (const void *key, size_t size)
  : ctx_{EVP_md5(), key, size}
{}

template <>
void hmac_t<md5>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<md5>::finish (void *result, size_t)
{
  ctx_.finish(result);
}

template <>
void hmac_t<md5>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  __bits::hmac_ctx_t::one_shot(EVP_md5(),
    key, key_size,
    data, data_size,
    result
  );
}

// sha1 {{{1

template <>
hmac_t<sha1>::hmac_t (const void *key, size_t size)
  : ctx_{EVP_sha1(), key, size}
{}

template <>
void hmac_t<sha1>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha1>::finish (void *result, size_t)
{
  ctx_.finish(result);
}

template <>
void hmac_t<sha1>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  __bits::hmac_ctx_t::one_shot(EVP_sha1(),
    key, key_size,
    data, data_size,
    result
  );
}

// sha256 {{{1

template <>
hmac_t<sha256>::hmac_t (const void *key, size_t size)
  : ctx_{EVP_sha256(), key, size}
{}

template <>
void hmac_t<sha256>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha256>::finish (void *result, size_t)
{
  ctx_.finish(result);
}

template <>
void hmac_t<sha256>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  __bits::hmac_ctx_t::one_shot(EVP_sha256(),
    key, key_size,
    data, data_size,
    result
  );
}

// sha384 {{{1

template <>
hmac_t<sha384>::hmac_t (const void *key, size_t size)
  : ctx_{EVP_sha384(), key, size}
{}

template <>
void hmac_t<sha384>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha384>::finish (void *result, size_t)
{
  ctx_.finish(result);
}

template <>
void hmac_t<sha384>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  __bits::hmac_ctx_t::one_shot(EVP_sha384(),
    key, key_size,
    data, data_size,
    result
  );
}

// sha512 {{{1

template <>
hmac_t<sha512>::hmac_t (const void *key, size_t size)
  : ctx_{EVP_sha512(), key, size}
{}

template <>
void hmac_t<sha512>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha512>::finish (void *result, size_t)
{
  ctx_.finish(result);
}

template <>
void hmac_t<sha512>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t)
{
  __bits::hmac_ctx_t::one_shot(EVP_sha512(),
    key, key_size,
    data, data_size,
    result
  );
}

// }}}1

#elif __sal_os_windows

// md5 {{{1

template <>
hmac_t<md5>::hmac_t (const void *key, size_t size)
{
  ctx_ = __bits::digest_t::make<md5, true>(key, size);
}

template <>
void hmac_t<md5>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<md5>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hmac_t<md5>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hmac(
    __bits::digest_t::factory<md5, true>(),
    key, key_size,
    data, data_size,
    result, result_size
  );
}

// sha1 {{{1

template <>
hmac_t<sha1>::hmac_t (const void *key, size_t size)
{
  ctx_ = __bits::digest_t::make<sha1, true>(key, size);
}

template <>
void hmac_t<sha1>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha1>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hmac_t<sha1>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hmac(
    __bits::digest_t::factory<sha1, true>(),
    key, key_size,
    data, data_size,
    result, result_size
  );
}

// sha256 {{{1

template <>
hmac_t<sha256>::hmac_t (const void *key, size_t size)
{
  ctx_ = __bits::digest_t::make<sha256, true>(key, size);
}

template <>
void hmac_t<sha256>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha256>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hmac_t<sha256>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hmac(
    __bits::digest_t::factory<sha256, true>(),
    key, key_size,
    data, data_size,
    result, result_size
  );
}

// sha384 {{{1

template <>
hmac_t<sha384>::hmac_t (const void *key, size_t size)
{
  ctx_ = __bits::digest_t::make<sha384, true>(key, size);
}

template <>
void hmac_t<sha384>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha384>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hmac_t<sha384>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hmac(
    __bits::digest_t::factory<sha384, true>(),
    key, key_size,
    data, data_size,
    result, result_size
  );
}

// sha512 {{{1

template <>
hmac_t<sha512>::hmac_t (const void *key, size_t size)
{
  ctx_ = __bits::digest_t::make<sha512, true>(key, size);
}

template <>
void hmac_t<sha512>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}

template <>
void hmac_t<sha512>::finish (void *result, size_t size)
{
  ctx_.finish(result, size);
}

template <>
void hmac_t<sha512>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *result, size_t result_size)
{
  __bits::digest_t::hmac(
    __bits::digest_t::factory<sha512, true>(),
    key, key_size,
    data, data_size,
    result, result_size
  );
}

// }}}1

#endif


template class hmac_t<md5>;
template class hmac_t<sha1>;
template class hmac_t<sha256>;
template class hmac_t<sha384>;
template class hmac_t<sha512>;


} // namespace crypto


__sal_end
