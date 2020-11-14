#include <iostream>
#include <SDL.h>
#include <memory>
#include <iomanip>

#include "linux/interface.h"
#include "linux/windows/misc.h"
#include "linux/data.h"
#include "linux/paddle.h"

#include "frontends/common2/configuration.h"
#include "frontends/common2/utils.h"
#include "frontends/common2/programoptions.h"
#include "frontends/common2/timer.h"
#include "frontends/sa2/emulator.h"
#include "frontends/sa2/gamepad.h"
#include "frontends/sa2/sdirectsound.h"
#include "frontends/sa2/utils.h"

#include "StdAfx.h"
#include "Common.h"
#include "CardManager.h"
#include "AppleWin.h"
#include "Disk.h"
#include "Mockingboard.h"
#include "SoundCore.h"
#include "Harddisk.h"
#include "Speaker.h"
#include "Log.h"
#include "CPU.h"
#include "Frame.h"
#include "Memory.h"
#include "LanguageCard.h"
#include "MouseInterface.h"
#include "ParallelPrinter.h"
#include "Video.h"
#include "NTSC.h"
#include "SaveState.h"
#include "RGBMonitor.h"
#include "Riff.h"


namespace
{
  void initialiseEmulator()
  {
    LogFileOutput("Initialisation\n");

    ImageInitialize();
    g_bFullSpeed = false;
  }

  void loadEmulator()
  {
    LoadConfiguration();
    CheckCpu();
    SetWindowTitle();
    FrameRefreshStatus(DRAW_LEDS | DRAW_BUTTON_DRIVES);

    DSInit();
    MB_Initialize();
    SpkrInitialize();

    MemInitialize();
    VideoInitialize();
    VideoSwitchVideocardPalette(RGB_GetVideocard(), GetVideoType());

    GetCardMgr().GetDisk2CardMgr().Reset();
    HD_Reset();
  }

  void applyOptions(const EmulatorOptions & options)
  {
    if (options.log)
    {
      LogInit();
    }

    bool disksOk = true;
    if (!options.disk1.empty())
    {
      const bool ok = DoDiskInsert(SLOT6, DRIVE_1, options.disk1, options.createMissingDisks);
      disksOk = disksOk && ok;
      LogFileOutput("Init: DoDiskInsert(D1), res=%d\n", ok);
    }

    if (!options.disk2.empty())
    {
      const bool ok = DoDiskInsert(SLOT6, DRIVE_2, options.disk2, options.createMissingDisks);
      disksOk = disksOk && ok;
      LogFileOutput("Init: DoDiskInsert(D2), res=%d\n", ok);
    }

    if (!options.snapshot.empty())
    {
      setSnapshotFilename(options.snapshot);
    }

    Paddle::setSquaring(options.squaring);
  }

  void stopEmulator()
  {
    CMouseInterface* pMouseCard = GetCardMgr().GetMouseCard();
    if (pMouseCard)
    {
      pMouseCard->Reset();
    }
    MemDestroy();
  }

  void uninitialiseEmulator()
  {
    SpkrDestroy();
    MB_Destroy();
    DSUninit();

    HD_Destroy();
    PrintDestroy();
    CpuDestroy();

    GetCardMgr().GetDisk2CardMgr().Destroy();
    ImageDestroy();
    LogDone();
    RiffFinishWriteFile();
  }

  int getRefreshRate()
  {
    SDL_DisplayMode current;

    const int should_be_zero = SDL_GetCurrentDisplayMode(0, &current);

    if (should_be_zero)
    {
      throw std::runtime_error(SDL_GetError());
    }

    return current.refresh_rate;
  }

  struct Data
  {
    Emulator * emulator;
    SDL_mutex * mutex;
    Timer * timer;
  };

  Uint32 emulator_callback(Uint32 interval, void *param)
  {
    Data * data = static_cast<Data *>(param);
    SDL_LockMutex(data->mutex);

    data->timer->tic();
    const int uCyclesToExecute = int(g_fCurrentCLK6502 * interval * 0.001);
    data->emulator->executeCycles(uCyclesToExecute);
    data->timer->toc();

    SDL_UnlockMutex(data->mutex);
    return interval;
  }

}

int MessageBox(HWND, const char * text, const char * caption, UINT type)
{
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, caption, text, nullptr);
  return IDOK;
}

void FrameDrawDiskLEDS(HDC x)
{
}

void FrameDrawDiskStatus(HDC x)
{
}

void FrameRefreshStatus(int x, bool)
{
}

void run_sdl(int argc, const char * argv [])
{
  EmulatorOptions options;
  options.memclear = g_nMemoryClearType;
  const bool run = getEmulatorOptions(argc, argv, "SDL2", options);

  if (!run)
    return;

  if (options.log)
  {
    LogInit();
  }

  InitializeRegistry(options);

  Paddle::instance().reset(new Gamepad(0));

  g_nMemoryClearType = options.memclear;

#ifdef RIFF_SPKR
  RiffInitWriteFile("/tmp/Spkr.wav", SPKR_SAMPLE_RATE, 1);
#endif
#ifdef RIFF_MB
  RiffInitWriteFile("/tmp/Mockingboard.wav", 44100, 2);
#endif

  initialiseEmulator();
  loadEmulator();

  applyOptions(options);

  const int width = GetFrameBufferWidth();
  const int height = GetFrameBufferHeight();
  const int sw = GetFrameBufferBorderlessWidth();
  const int sh = GetFrameBufferBorderlessHeight();

  std::cerr << std::fixed << std::setprecision(2);

  std::shared_ptr<SDL_Window> win(SDL_CreateWindow(g_pAppTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, sw, sh, SDL_WINDOW_SHOWN), SDL_DestroyWindow);
  if (!win)
  {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    return;
  }

  std::shared_ptr<SDL_Renderer> ren(SDL_CreateRenderer(win.get(), options.sdlDriver, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC), SDL_DestroyRenderer);
  if (!ren)
  {
    std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    return;
  }

  const Uint32 format = SDL_PIXELFORMAT_ARGB8888;
  printRendererInfo(std::cerr, ren, format, options.sdlDriver);

  std::shared_ptr<SDL_Texture> tex(SDL_CreateTexture(ren.get(), format, SDL_TEXTUREACCESS_STATIC, width, height), SDL_DestroyTexture);

  const int fps = getRefreshRate();
  std::cerr << "Video refresh rate: " << fps << " Hz, " << 1000.0 / fps << " ms" << std::endl;
  Emulator emulator(win, ren, tex);

  Timer global;
  Timer updateTextureTimer;
  Timer refreshScreenTimer;
  Timer cpuTimer;
  Timer eventTimer;

  const std::string globalTag = ". .";
  std::string updateTextureTimerTag, refreshScreenTimerTag, cpuTimerTag, eventTimerTag;

  if (options.multiThreaded)
  {
    refreshScreenTimerTag = "0 .";
    cpuTimerTag           = "1 M";
    eventTimerTag         = "0 M";
    if (options.looseMutex)
    {
      updateTextureTimerTag = "0 .";
    }
    else
    {
      updateTextureTimerTag = "0 M";
    }

    std::shared_ptr<SDL_mutex> mutex(SDL_CreateMutex(), SDL_DestroyMutex);

    Data data;
    data.mutex = mutex.get();
    data.emulator = &emulator;
    data.timer = &cpuTimer;

    const SDL_TimerID timer = SDL_AddTimer(options.timerInterval, emulator_callback, &data);

    bool quit = false;
    do
    {
      SDL_LockMutex(data.mutex);

      eventTimer.tic();
      SDirectSound::writeAudio();
      emulator.processEvents(quit);
      eventTimer.toc();

      if (options.looseMutex)
      {
	// loose mutex
	// unlock early and let CPU run again in the timer callback
	SDL_UnlockMutex(data.mutex);
	// but the texture will be updated concurrently with the CPU updating the video buffer
	// pixels are not atomic, so a pixel error could happen (if pixel changes while being read)
	// on the positive side this will release pressure from CPU and allow for more parallelism
      }

      updateTextureTimer.tic();
      const SDL_Rect rect = emulator.updateTexture();
      updateTextureTimer.toc();

      if (!options.looseMutex)
      {
	// safe mutex, only unlock after texture has been updated
	// this will stop the CPU for longer
	SDL_UnlockMutex(data.mutex);
      }

      refreshScreenTimer.tic();
      emulator.refreshVideo(rect);
      refreshScreenTimer.toc();

    } while (!quit);

    SDL_RemoveTimer(timer);
    // if the following enough to make sure the timer has finished
    // and wont be called again?
    SDL_LockMutex(data.mutex);
    SDL_UnlockMutex(data.mutex);
  }
  else
  {
    refreshScreenTimerTag = "0 .";
    cpuTimerTag           = "0 .";
    eventTimerTag         = "0 .";
    updateTextureTimerTag = "0 .";

    bool quit = false;
    const int uCyclesToExecute = int(g_fCurrentCLK6502 / fps);

    do
    {
      eventTimer.tic();
      SDirectSound::writeAudio();
      emulator.processEvents(quit);
      eventTimer.toc();

      cpuTimer.tic();
      emulator.executeCycles(uCyclesToExecute);
      cpuTimer.toc();

      updateTextureTimer.tic();
      const SDL_Rect rect = emulator.updateTexture();
      updateTextureTimer.toc();

      refreshScreenTimer.tic();
      emulator.refreshVideo(rect);
      refreshScreenTimer.toc();
    } while (!quit);
  }

  global.toc();

  const char sep[] = "], ";
  std::cerr << "Global:  [" << globalTag << sep << global << std::endl;
  std::cerr << "Events:  [" << eventTimerTag << sep << eventTimer << std::endl;
  std::cerr << "Texture: [" << updateTextureTimerTag << sep << updateTextureTimer << std::endl;
  std::cerr << "Screen:  [" << refreshScreenTimerTag << sep << refreshScreenTimer << std::endl;
  std::cerr << "CPU:     [" << cpuTimerTag << sep << cpuTimer << std::endl;

  const double timeInSeconds = global.getTimeInSeconds();
  const double actualClock = g_nCumulativeCycles / timeInSeconds;
  std::cerr << "Expected clock: " << g_fCurrentCLK6502 << " Hz, " << g_nCumulativeCycles / g_fCurrentCLK6502 << " s" << std::endl;
  std::cerr << "Actual clock:   " << actualClock << " Hz, " << timeInSeconds << " s" << std::endl;

  SDirectSound::stop();
  stopEmulator();
  uninitialiseEmulator();
}

int main(int argc, const char * argv [])
{
  //First we need to start up SDL, and make sure it went ok
  const Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO | SDL_INIT_TIMER;
  if (SDL_Init(flags) != 0)
  {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  int exit = 0;

  try
  {
    run_sdl(argc, argv);
  }
  catch (const std::exception & e)
  {
    exit = 2;
    std::cerr << e.what() << std::endl;
  }


  // this must happen BEFORE the SDL_Quit() as otherwise we have a double free (of the game controller).
  Paddle::instance().reset();
  SDL_Quit();

  return exit;
}