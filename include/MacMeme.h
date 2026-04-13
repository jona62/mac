#ifndef MAC_MEME_H
#define MAC_MEME_H

#include <iomanip>
#include <memory>
#include <string>
#include <sstream>

namespace meme {

    class MacMeme {
    public:
        std::string templateName;
        std::string topText;
        std::string bottomText;

        MacMeme(const std::string& tmpl, const std::string& top, const std::string& bottom)
            : templateName(tmpl), topText(top), bottomText(bottom) {}

        std::string toString() const {
            std::ostringstream out;
            int width = 30;
            std::string border(width, '-');
            out << "+" << border << "+\n";
            out << "| " << std::left << std::setw(width - 2) << templateName << " |\n";
            out << "+" << border << "+\n";
            out << "| " << std::left << std::setw(width - 2) << topText << " |\n";
            out << "|" << std::string(width, ' ') << "|\n";
            out << "| " << std::left << std::setw(width - 2) << bottomText << " |\n";
            out << "+" << border << "+";
            return out.str();
        }

        std::shared_ptr<MacMeme> remix(const std::string& newTop, const std::string& newBottom) const {
            return std::make_shared<MacMeme>(templateName, newTop, newBottom);
        }
    };

} // namespace meme

#endif // MAC_MEME_H
