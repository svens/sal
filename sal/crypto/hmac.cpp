#include <sal/crypto/hmac.hpp>


__sal_begin


namespace crypto {


#if __sal_os_darwin //{{{1


namespace {

template <typename Algorithm> constexpr CCHmacAlgorithm algorithm_id_v = {};
template <> constexpr CCHmacAlgorithm algorithm_id_v<md5> = kCCHmacAlgMD5;
template <> constexpr CCHmacAlgorithm algorithm_id_v<sha1> = kCCHmacAlgSHA1;
template <> constexpr CCHmacAlgorithm algorithm_id_v<sha256> = kCCHmacAlgSHA256;
template <> constexpr CCHmacAlgorithm algorithm_id_v<sha384> = kCCHmacAlgSHA384;
template <> constexpr CCHmacAlgorithm algorithm_id_v<sha512> = kCCHmacAlgSHA512;

} // namespace


template <typename Algorithm>
hmac_t<Algorithm>::hmac_t (const void *key, size_t size)
{
  ::CCHmacInit(&ctx_[0], algorithm_id_v<Algorithm>, key, size);
  ctx_[1] = ctx_[0];
}


template <typename Algorithm>
void hmac_t<Algorithm>::update (const void *data, size_t size)
{
  ::CCHmacUpdate(&ctx_[0], data, size);
}


template <typename Algorithm>
void hmac_t<Algorithm>::finish (void *digest, size_t)
{
  ::CCHmacFinal(&ctx_[0], digest);
  ctx_[0] = ctx_[1];
}


template <typename Algorithm>
void hmac_t<Algorithm>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *digest, size_t)
{
  ::CCHmac(algorithm_id_v<Algorithm>, key, key_size, data, data_size, digest);
}


#elif __sal_os_linux //{{{1


namespace {

template <typename Algorithm> const EVP_MD *algorithm_id_v = {};
template <> const EVP_MD *algorithm_id_v<md5> = EVP_md5();
template <> const EVP_MD *algorithm_id_v<sha1> = EVP_sha1();
template <> const EVP_MD *algorithm_id_v<sha256> = EVP_sha256();
template <> const EVP_MD *algorithm_id_v<sha384> = EVP_sha384();
template <> const EVP_MD *algorithm_id_v<sha512> = EVP_sha512();

} // namespace


namespace __bits {


hmac_ctx_t::hmac_ctx_t (const EVP_MD *evp, const void *key, size_t size) noexcept
  : ctx(std::make_unique<HMAC_CTX>())
{
  if (!key)
  {
    key = "";
    size = 0U;
  }
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


void hmac_ctx_t::finish (void *digest)
{
  ::HMAC_Final(ctx.get(), static_cast<uint8_t *>(digest), nullptr);
  ::HMAC_Init_ex(ctx.get(), nullptr, 0U, nullptr, nullptr);
}


void hmac_ctx_t::one_shot (const EVP_MD *evp,
  const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *digest)
{
  if (!key)
  {
    key = "";
    key_size = 0U;
  }
  ::HMAC(evp,
    static_cast<const uint8_t *>(key), key_size,
    static_cast<const uint8_t *>(data), data_size,
    static_cast<uint8_t *>(digest), nullptr
  );
}


} // namespace __bits


template <typename Algorithm>
hmac_t<Algorithm>::hmac_t (const void *key, size_t size)
  : ctx_{algorithm_id_v<Algorithm>, key, size}
{}


template <typename Algorithm>
void hmac_t<Algorithm>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}


template <typename Algorithm>
void hmac_t<Algorithm>::finish (void *digest, size_t)
{
  ctx_.finish(digest);
}


template <typename Algorithm>
void hmac_t<Algorithm>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *digest, size_t)
{
  __bits::hmac_ctx_t::one_shot(algorithm_id_v<Algorithm>,
    key, key_size,
    data, data_size,
    digest
  );
}


#elif __sal_os_windows //{{{1


template <typename Algorithm>
hmac_t<Algorithm>::hmac_t (const void *key, size_t size)
{
  ctx_ = __bits::context_t::make<Algorithm, true>(key, size);
}


template <typename Algorithm>
void hmac_t<Algorithm>::update (const void *data, size_t size)
{
  ctx_.update(data, size);
}


template <typename Algorithm>
void hmac_t<Algorithm>::finish (void *digest, size_t size)
{
  ctx_.finish(digest, size);
}


template <typename Algorithm>
void hmac_t<Algorithm>::one_shot (const void *key, size_t key_size,
  const void *data, size_t data_size,
  void *digest, size_t digest_length)
{
  __bits::context_t::hmac(
    __bits::context_t::factory<Algorithm, true>(),
    key, key_size,
    data, data_size,
    digest, digest_length
  );
}


#endif //}}}


template class hmac_t<md5>;
template class hmac_t<sha1>;
template class hmac_t<sha256>;
template class hmac_t<sha384>;
template class hmac_t<sha512>;


} // namespace crypto


__sal_end
