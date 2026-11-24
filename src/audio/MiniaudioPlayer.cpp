#include "audio/MiniaudioPlayer.hpp"

#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <set>
#include <thread>
#include <utility>

#include <miniaudio.h>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace rogue {

namespace {

// Selama inisialisasi perangkat, library suara sistem (ALSA/JACK) bisa menulis
// pesan galat ke stderr dan merusak tampilan TUI bila tidak ada perangkat.
// Alihkan stderr ke /dev/null sebentar; dikembalikan saat objek hilang.
class QuietStderr {
 public:
  QuietStderr() {
#ifndef _WIN32
    saved_ = ::dup(2);
    const int devNull = ::open("/dev/null", O_WRONLY);
    if (saved_ >= 0 && devNull >= 0) ::dup2(devNull, 2);
    if (devNull >= 0) ::close(devNull);
#endif
  }
  ~QuietStderr() {
#ifndef _WIN32
    if (saved_ >= 0) {
      ::dup2(saved_, 2);
      ::close(saved_);
    }
#endif
  }
  QuietStderr(const QuietStderr&) = delete;
  QuietStderr& operator=(const QuietStderr&) = delete;

 private:
#ifndef _WIN32
  int saved_ = -1;
#endif
};

}  // namespace

struct MiniaudioPlayer::Impl {
  Impl(ShuffleQueue q, MiniaudioOptions o) : queue(std::move(q)), options(o) {}

  ShuffleQueue queue;  // hanya disentuh thread pengelola setelah start()
  MiniaudioOptions options;
  std::thread worker;

  mutable std::mutex mutex;
  std::condition_variable cv;
  bool started = false;        // dijaga mutex
  bool stopRequested = false;  // dijaga mutex
  bool songEnded = false;      // dijaga mutex
  std::string current;         // dijaga mutex

  static void onSongEnd(void* user, ma_sound*) {
    auto* self = static_cast<Impl*>(user);
    {
      std::lock_guard<std::mutex> lock(self->mutex);
      self->songEnded = true;
    }
    self->cv.notify_all();
  }

  bool shouldStop() {
    std::lock_guard<std::mutex> lock(mutex);
    return stopRequested;
  }

  void run() {
    ma_context context;
    bool haveContext = false;
    ma_engine engine;
    bool engineOk = false;
    {
      QuietStderr quiet;
      ma_engine_config config = ma_engine_config_init();
      if (options.useNullBackend) {
        const ma_backend backends[] = {ma_backend_null};
        ma_context_config contextConfig = ma_context_config_init();
        if (ma_context_init(backends, 1, &contextConfig, &context) != MA_SUCCESS) return;
        haveContext = true;
        config.pContext = &context;
      }
      engineOk = ma_engine_init(&config, &engine) == MA_SUCCESS;
    }
    if (!engineOk) {
      if (haveContext) ma_context_uninit(&context);
      return;  // tanpa perangkat suara: diam
    }

    // Lagu yang gagal dibuka dicatat; tidak dicoba lagi. Berhenti hanya bila
    // SEMUA lagu berbeda sudah gagal (antrian menyisipkan ulang lagu rusak,
    // jadi hitungan "gagal berturut-turut" bisa menyesatkan).
    std::set<std::filesystem::path> failed;
    const std::size_t total = queue.size();
    while (!shouldStop()) {
      auto song = queue.next();
      if (!song) break;

      if (failed.count(*song) != 0) continue;

      ma_sound sound;
#ifdef _WIN32
      const ma_result r = ma_sound_init_from_file_w(&engine, song->c_str(), MA_SOUND_FLAG_STREAM,
                                                    nullptr, nullptr, &sound);
#else
      const ma_result r = ma_sound_init_from_file(&engine, song->c_str(), MA_SOUND_FLAG_STREAM,
                                                  nullptr, nullptr, &sound);
#endif
      if (r != MA_SUCCESS) {
        failed.insert(*song);
        if (failed.size() >= total) break;  // semua rusak: berhenti diam-diam
        continue;
      }

      {
        std::lock_guard<std::mutex> lock(mutex);
        songEnded = false;
        current = song->filename().string();
      }
      ma_sound_set_end_callback(&sound, &Impl::onSongEnd, this);
      ma_sound_start(&sound);

      {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return songEnded || stopRequested; });
      }
      ma_sound_uninit(&sound);  // tanpa memegang mutex (callback memakainya)
      {
        std::lock_guard<std::mutex> lock(mutex);
        current.clear();
      }
    }

    ma_engine_uninit(&engine);
    if (haveContext) ma_context_uninit(&context);
  }
};

MiniaudioPlayer::MiniaudioPlayer(ShuffleQueue queue, MiniaudioOptions options)
    : impl_(std::make_unique<Impl>(std::move(queue), options)) {}

MiniaudioPlayer::~MiniaudioPlayer() { stop(); }

void MiniaudioPlayer::start() {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  if (impl_->started || impl_->stopRequested) return;
  impl_->started = true;
  impl_->worker = std::thread([impl = impl_.get()] { impl->run(); });
}

void MiniaudioPlayer::stop() {
  {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->stopRequested = true;
  }
  impl_->cv.notify_all();
  if (impl_->worker.joinable()) impl_->worker.join();
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->current.clear();
}

std::string MiniaudioPlayer::nowPlaying() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->current;
}

}  // namespace rogue
