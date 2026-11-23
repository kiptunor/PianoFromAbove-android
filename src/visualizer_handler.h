#ifndef VISUALIZER_ENTRY_H
#define VISUALIZER_ENTRY_H


class VisualizerHandler
{
  public:
    bool keep_running;
    enum Type
    {
        Legacy = 1, // SDL3 Renderer
        GPU,    // SDL_gpu Renderer
        Custom
    };
    VisualizerHandler();
    ~VisualizerHandler();

    bool isDesktopSession();

    void Render();

    void setType(Type type);
    static void shutdown();
    Type getType() const;

  private:
      const char        *video_driver;
};

#endif