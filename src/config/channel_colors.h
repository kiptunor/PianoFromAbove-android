#ifndef CHANNEL_COLORS_H
#define CHANNEL_COLORS_H

#include <string>














class ChannelColors
{
  public:
    static unsigned int *getChannelColors(std::string filename);
    static bool          more_channel_colors;
};

#endif // CHANNEL_COLORS_H
