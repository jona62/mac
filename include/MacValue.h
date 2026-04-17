#ifndef MAC_VALUE_H
#define MAC_VALUE_H

#include <memory>               // shared_ptr
#include <string>               // string
#include <variant>              // variant, monostate (MacValue type)

namespace callable {
    class MacCallable;
}

namespace instance {
    class MacInstance;
}

namespace collection {
    class MacArray;
    class MacMap;
}

namespace meme {
    class MacMeme;
    class MacGif;
    class MacTimeline;
    class RenderSurface;
}

namespace value {
    using MacValue = std::variant<std::string, double, bool, std::monostate,
                                  std::shared_ptr<callable::MacCallable>,
                                  std::shared_ptr<instance::MacInstance>,
                                  std::shared_ptr<collection::MacArray>,
                                  std::shared_ptr<collection::MacMap>,
                                  std::shared_ptr<meme::RenderSurface>,
                                  std::shared_ptr<meme::MacMeme>,
                                  std::shared_ptr<meme::MacGif>,
                                  std::shared_ptr<meme::MacTimeline>>;
}

#endif // MAC_VALUE_H
