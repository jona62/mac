#ifndef MAC_VALUE_H
#define MAC_VALUE_H

#include <memory>
#include <string>
#include <variant>

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
}

namespace value {
    using MacValue = std::variant<std::string, double, bool, std::monostate,
                                  std::shared_ptr<callable::MacCallable>,
                                  std::shared_ptr<instance::MacInstance>,
                                  std::shared_ptr<collection::MacArray>,
                                  std::shared_ptr<collection::MacMap>,
                                  std::shared_ptr<meme::MacMeme>,
                                  std::shared_ptr<meme::MacGif>,
                                  std::shared_ptr<meme::MacTimeline>>;
}

#endif // MAC_VALUE_H
