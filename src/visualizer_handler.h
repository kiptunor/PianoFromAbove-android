#ifndef VISUALIZER_ENTRY_H
#define VISUALIZER_ENTRY_H


class VisualizerHandler
{
  public:
    enum Type
    {
        Legacy = 1, // SDL3 Renderer
        GPU,        // SDL_gpu Renderer
        Custom
    };
    VisualizerHandler();
    ~VisualizerHandler();

    void        setType(Type type);
    static void shutdown();
    Type        getType() const;
};

#endif