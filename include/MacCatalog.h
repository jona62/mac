#ifndef MAC_CATALOG_H
#define MAC_CATALOG_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>  // For curated catalog metadata lookup tables
#include <unordered_set>
#include <vector>

#include "GifLimits.h"
#include "MacClass.h"
#include "MacTemplateRegistry.h"
#include "NativeRegistry.h"
#include "nlohmann/json.hpp"

#ifndef MAC_VERSION
#define MAC_VERSION "unknown"
#endif

namespace mac_catalog {

    using json = nlohmann::json;

    struct TextZone {
        std::string id;
        std::string label;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        std::string anchor;
        std::string fontSize;
        std::string guidance;
    };

    struct TemplateEntry {
        std::string id;
        std::string name;
        std::string category;
        std::string description;
        std::string bestFor;
        std::string source;
        std::string assetCategory;
        std::string assetPath;
        std::vector<std::string> tags;
        std::vector<std::string> moods;
        std::vector<std::string> subjects;
        std::vector<std::string> aliases;
        std::string captionGuidance;
        std::string comedicName;
        std::string comedicRead;
        std::vector<std::string> antiPatterns;
        std::vector<TextZone> textZones;
    };

    struct TemplateMetadata {
        std::string description;
        std::string bestFor;
        std::vector<std::string> tags;
        std::vector<std::string> moods;
        std::vector<std::string> subjects;
        std::vector<std::string> aliases;
        std::string captionGuidance;
        std::string comedicName;
        std::string comedicRead;
        std::vector<std::string> antiPatterns;
        std::vector<TextZone> textZones;
    };

    inline std::string lower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    inline std::string titleizeStem(std::string value) {
        std::replace(value.begin(), value.end(), '_', ' ');
        bool nextUpper = true;
        for (auto& ch : value) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (std::isspace(c) || ch == '-') {
                nextUpper = true;
                continue;
            }
            ch = static_cast<char>(nextUpper ? std::toupper(c) : std::tolower(c));
            nextUpper = false;
        }
        return value;
    }

    inline std::vector<std::string> stemWords(std::string value) {
        std::replace(value.begin(), value.end(), '-', '_');
        std::vector<std::string> out;
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, '_')) {
            if (!item.empty()) out.push_back(lower(item));
        }
        return out;
    }

    inline bool hasWord(const std::vector<std::string>& words, const std::string& word) {
        return std::find(words.begin(), words.end(), word) != words.end();
    }

    inline void addUnique(std::vector<std::string>& values, const std::string& value) {
        if (value.empty()) return;
        if (std::find(values.begin(), values.end(), value) == values.end()) {
            values.push_back(value);
        }
    }

    inline void addAllUnique(std::vector<std::string>& values, const std::vector<std::string>& more) {
        for (const auto& value : more) addUnique(values, value);
    }

    inline std::vector<std::string> uniqueValues(std::vector<std::string> values) {
        std::vector<std::string> out;
        for (const auto& value : values) addUnique(out, value);
        return out;
    }

    inline TemplateMetadata fallbackAssetMetadata(const std::string& stem, const std::string& category) {
        auto words = stemWords(stem);
        std::vector<std::string> tags = {category, "meme", "reaction"};
        std::vector<std::string> moods;
        std::vector<std::string> subjects;
        std::vector<std::string> aliases = {stem, titleizeStem(stem)};

        addAllUnique(tags, words);
        for (const auto& word : words) {
            if (word == "cat" || word == "dog" || word == "ferret" || word == "cow" ||
                word == "dolphin" || word == "horse" || word == "trex" || word == "t" ||
                word == "rex") {
                addUnique(subjects, word == "t" || word == "rex" ? "t-rex" : word);
            } else if (word == "desk" || word == "worker" || word == "office" ||
                       word == "terminal" || word == "kitchen" || word == "window" ||
                       word == "stage" || word == "press" || word == "conference" ||
                       word == "alarm" || word == "clock" || word == "car" ||
                       word == "beach" || word == "hill" || word == "room" ||
                       word == "throne" || word == "fire" || word == "explosion" ||
                       word == "group" || word == "stare") {
                addUnique(subjects, word);
            } else if (word == "spongebob" || word == "squidward" || word == "shrek" ||
                       word == "drake" || word == "jordan" || word == "pepe" ||
                       word == "doge" || word == "gary" || word == "krusty") {
                addUnique(subjects, word);
            }
        }

        if (hasWord(words, "angry") || hasWord(words, "fire") || hasWord(words, "explosion")) {
            addAllUnique(moods, {"panic", "chaos", "meltdown"});
        }
        if (hasWord(words, "crying") || hasWord(words, "despair") || hasWord(words, "sad")) {
            addAllUnique(moods, {"sad", "despair"});
        }
        if (hasWord(words, "side") || hasWord(words, "eye") || hasWord(words, "unimpressed") ||
            hasWord(words, "stare")) {
            addAllUnique(moods, {"skeptical", "deadpan"});
        }
        if (hasWord(words, "bliss") || hasWord(words, "sun") || hasWord(words, "beach")) {
            addAllUnique(moods, {"calm", "joy"});
        }
        if (hasWord(words, "empty")) {
            addAllUnique(moods, {"awkward", "absence"});
        }
        if (moods.empty()) addUnique(moods, "reaction");

        std::string description = "Reaction meme asset showing " + titleizeStem(stem) + ".";
        std::string bestFor = "Readable reaction beats, labels, and short punchlines.";
        std::string captionGuidance = "Prefer top or bottom captions; keep center text very short and avoid covering faces or the primary subject.";
        std::string comedicRead = "A flexible reaction image; make the caption specific enough to supply the joke.";
        return {
            description,
            bestFor,
            uniqueValues(tags),
            uniqueValues(moods),
            uniqueValues(subjects),
            uniqueValues(aliases),
            captionGuidance,
            "",
            comedicRead,
            {"generic labels without a situation", "center text covering the subject"},
        };
    }

    inline const std::unordered_map<std::string, TemplateMetadata>& curatedAssetMetadata() {
        static const std::unordered_map<std::string, TemplateMetadata> metadata = {
            {"meme.angry_alarm_clock", {
                "Angry alarm clock character with intense wake-up energy.",
                "Sleep, Monday, procrastination, bargaining, and tiny daily betrayals.",
                {"meme", "reaction", "alarm", "clock", "morning", "sleep"},
                {"panic", "anger", "dread"},
                {"alarm", "clock"},
                {"alarm clock", "angry alarm", "morning alarm"},
                "Use bottom captions for the complaint; avoid center text over the clock face.",
                "The 7 AM Betrayal",
                "A tiny plastic tyrant announcing that your previous self made bad choices.",
                {"generic Monday bad text", "center captions over the clock face"},
            }},
            {"meme.barbie_car_fire", {
                "Pink toy car scene consumed by very unserious-looking fire.",
                "Cute aesthetics colliding with total crisis, overdramatic errands, and glamorous disaster.",
                {"meme", "reaction", "barbie", "car", "fire", "cute", "crisis"},
                {"panic", "chaos", "meltdown", "absurd"},
                {"car", "fire"},
                {"barbie car", "pink car fire", "cute disaster"},
                "Use bottom captions; keep the fire and car readable.",
                "Cute Car, Total Crisis",
                "The visual joke is that everything is on fire but the branding stayed adorable.",
                {"flat disaster labels", "effects so heavy the pink car disappears"},
            }},
            {"meme.bathroom_throne", {
                "Bathroom scene with absurd throne energy.",
                "Private revelations, fake luxury, shower thoughts, and decisions made with unearned authority.",
                {"meme", "reaction", "bathroom", "throne", "privacy", "decision"},
                {"deadpan", "absurd", "smug"},
                {"bathroom", "throne"},
                {"bathroom throne", "toilet throne", "private kingdom"},
                "Prefer a short bottom caption; avoid making the scene feel too literal.",
                "Executive Bathroom Decision",
                "Someone has clearly mistaken solitude for leadership experience.",
                {"gross-out captions as the whole joke", "long centered captions"},
            }},
            {"meme.beach_dog_sitting", {
                "Dog sitting alone on a beach with peaceful resigned energy.",
                "Calm acceptance, pretending everything is fine, solitude, vacation-brain jokes.",
                {"meme", "reaction", "dog", "beach", "calm", "resigned"},
                {"calm", "resigned", "deadpan"},
                {"dog", "beach"},
                {"beach dog", "resigned dog", "calm dog"},
                "Use bottom captions; keep text short so the quiet beach mood stays visible.",
                "Vacation Brain Has Left The Chat",
                "A dog quietly accepting a situation it absolutely does not understand.",
                {"busy captions that ruin the quiet", "loud chaos effects"},
            }},
            {"meme.bliss_ferret_stare", {
                "Small ferret staring with serene, empty-headed bliss.",
                "No-thoughts moments, tiny victories, harmless delusion, and being weirdly pleased.",
                {"meme", "reaction", "ferret", "bliss", "stare", "empty headed"},
                {"calm", "joy", "deadpan", "weird"},
                {"ferret", "stare"},
                {"bliss ferret", "ferret stare", "no thoughts ferret"},
                "Bottom captions work best; let the blank stare do half the joke.",
                "No Thoughts, Premium Bliss",
                "The joke lands when the caption reveals why this tiny creature is so peacefully wrong.",
                {"mean animal captions", "overexplaining the blank stare"},
            }},
            {"meme.cat_explosion", {
                "Cat in a chaotic explosion scene with maximum meltdown energy.",
                "Peak chaos, sudden realization, rage, disaster, and visual punchline moments.",
                {"meme", "reaction", "cat", "explosion", "chaos", "meltdown"},
                {"chaos", "panic", "meltdown"},
                {"cat", "explosion", "fire"},
                {"exploding cat", "chaos cat", "meltdown cat"},
                "Use short top or bottom captions; heavy effects are appropriate, but keep the cat readable.",
                "Tiny Problem, Biblical Consequences",
                "The correct asset when a minor inconvenience deserves an operatic disaster response.",
                {"using it for mild reactions", "deepfry so strong the cat vanishes"},
            }},
            {"meme.distracted_boyfriend", {
                "Classic distracted boyfriend scene with three clear role positions.",
                "Temptation, bad priorities, choosing the wrong thing, and comparison jokes.",
                {"meme", "template", "choice", "temptation", "comparison"},
                {"tempted", "awkward", "comic"},
                {"boyfriend", "girlfriend", "street"},
                {"distracted boyfriend", "temptation trio", "boyfriend looking back"},
                "Use short labels near each role; avoid generic ME/MY RESPONSIBILITIES unless requested.",
                "Bad Priorities Walk By",
                "A three-role temptation machine: stable choice, doomed observer, shiny mistake.",
                {"ME/MY RESPONSIBILITIES defaults", "long labels that need reading twice"},
            }},
            {"meme.dog_earbuds_bliss", {
                "Dog wearing earbuds in pure private-concert bliss.",
                "Ignoring problems, main-character errands, tiny joy, and pretending the world is fine.",
                {"meme", "reaction", "dog", "earbuds", "music", "bliss"},
                {"joy", "calm", "smug"},
                {"dog", "earbuds"},
                {"earbuds dog", "music dog", "bliss dog"},
                "Use bottom captions; avoid covering the face or earbuds.",
                "Main Character Errand Walk",
                "The mood is delusional confidence powered by one good song.",
                {"generic happy dog text", "sad captions that fight the image"},
            }},
            {"meme.drake_reaction_grid", {
                "Two-row Drake reaction grid with image panels on the left and blank label panels on the right.",
                "Reject/approve comparisons, bad plan vs better plan, and short two-option contrast jokes.",
                {"meme", "reaction", "drake", "grid", "comparison", "two panel", "label panel"},
                {"reaction", "judging", "comparison"},
                {"drake", "grid", "blank panels"},
                {"drake reaction grid", "drake grid", "drake approve reject"},
                "This image already contains a two-row grid. Do not use whole-canvas top/bottom captions. Put short positioned text in the right-side blank panels: reject_option near (540,170), approve_option near (540,530).",
                "Two Choices, One Obvious",
                "The joke is the contrast between the rejected idea and the suspiciously satisfying alternative.",
                {"whole-canvas top/bottom captions", "leaving the right label panels blank", "captions over Drake instead of the blank panels"},
                {
                    {"reject_option", "top-right label panel", 390, 40, 300, 260, "center", "sm", "Rejected option; keep to 3-7 words."},
                    {"approve_option", "bottom-right label panel", 390, 400, 300, 260, "center", "sm", "Preferred option or punchline; keep to 3-7 words."},
                },
            }},
            {"meme.evil_cat_throne", {
                "Cat seated like a tiny villain on a throne.",
                "Scheming, petty power, snack crimes, household tyranny, and mock-grand drama.",
                {"meme", "reaction", "cat", "throne", "villain", "scheming"},
                {"smug", "sinister", "petty"},
                {"cat", "throne"},
                {"evil cat", "cat throne", "villain cat"},
                "Bottom captions work best; one concise royal decree or consequence.",
                "The Snack Crime Overlord",
                "A tiny monarch preparing a household policy nobody voted for.",
                {"generic evil laugh captions", "overcrowding the throne area"},
            }},
            {"meme.girl_side_eye", {
                "Girl giving a strong side-eye reaction.",
                "Suspicion, judgment, awkward social moments, and disbelief.",
                {"meme", "reaction", "side-eye", "judgment", "social"},
                {"skeptical", "judging", "deadpan"},
                {"girl", "face"},
                {"side eye girl", "judgment stare", "suspicious girl"},
                "Use top or bottom captions; never cover the eyes with center text.",
                "The Receipt Has Been Noticed",
                "For the exact second someone hears a detail that changes the whole story.",
                {"covering the eyes", "generic 'sus' captions with no situation"},
            }},
            {"meme.lonely_desk_worker", {
                "Person alone at a desk with a monitor, quiet office mood.",
                "Work cycles, late-night focus, debugging, isolation, and calm-before-chaos setups.",
                {"meme", "reaction", "desk", "office", "computer", "debugging"},
                {"calm", "lonely", "focused"},
                {"desk", "worker", "computer"},
                {"desk worker", "lonely desk", "alone at computer"},
                "Use bottom captions or terminal-style text; keep the monitor and person visible.",
                "One Last Thing At 11:47 PM",
                "The setup frame for any bad idea that starts calmly and becomes everyone else's problem.",
                {"only developer jokes by default", "covering the desk/monitor subject"},
            }},
            {"meme.spongebob_group_stare", {
                "Group stare reaction with multiple characters looking toward the viewer.",
                "Group chat, social pressure, sudden attention, and everyone noticing at once.",
                {"meme", "reaction", "group", "stare", "social"},
                {"awkward", "judging", "surprised"},
                {"group", "faces"},
                {"group stare", "everyone staring", "spongebob group"},
                "Use bottom captions; avoid covering faces with centered text.",
                "The Group Chat Goes Silent",
                "Everyone has read the message; nobody wants to be the first to answer.",
                {"covering faces", "too much caption for a reaction beat"},
            }},
            {"meme.spongebob_war_room", {
                "Chaotic SpongeBob war-room scene with planning-board energy.",
                "Escalation, overthinking, debugging, strategy spirals, and panic planning.",
                {"meme", "reaction", "war room", "planning", "chaos"},
                {"tense", "panic", "spiral"},
                {"spongebob", "room", "board"},
                {"war room", "planning board", "spongebob war room"},
                "Use bottom captions; pair with mild desaturation or contrast for tense middle beats.",
                "The Plan Is Mostly Panic",
                "A room full of confidence that is technically just anxiety with stationery.",
                {"starting GIF arcs at maximum chaos", "covering the board/faces"},
            }},
            {"meme.squidward_window_stare", {
                "Squidward staring through a window with quiet longing and resignation.",
                "Hollow victory, watching from outside, loneliness, envy, and quiet aftermath.",
                {"meme", "reaction", "window", "squidward", "resignation"},
                {"resigned", "sad", "hollow"},
                {"squidward", "window"},
                {"squidward window", "window stare", "outside looking in"},
                "Use bottom captions with muted styling; avoid loud effects unless ironic.",
                "Watching Joy Happen Elsewhere",
                "A quiet little window into being technically fine and spiritually outside.",
                {"loud panic styling", "captioning over the face/window read"},
            }},
            {"meme.thousand_yard_stare", {
                "Blank thousand-yard stare with exhausted existential energy.",
                "Confusion, reality breaking, trauma, debugging rabbit holes, and quiet shock.",
                {"meme", "reaction", "stare", "existential", "confusion"},
                {"haunted", "confused", "dissociated"},
                {"face", "stare"},
                {"thousand yard stare", "blank stare", "haunted stare"},
                "Use bottom captions; keep text short and let the stare carry the joke.",
                "The Lights Are On But Nobody Saved",
                "The face of someone whose brain has opened too many tabs and lost the original one.",
                {"giant centered captions over the eyes", "using it for high-energy jokes"},
            }},
        };
        return metadata;
    }

    inline TemplateMetadata mergeMetadata(const TemplateMetadata& base, const TemplateMetadata& override) {
        TemplateMetadata result = base;
        if (!override.description.empty()) result.description = override.description;
        if (!override.bestFor.empty()) result.bestFor = override.bestFor;
        if (!override.captionGuidance.empty()) result.captionGuidance = override.captionGuidance;
        if (!override.comedicName.empty()) result.comedicName = override.comedicName;
        if (!override.comedicRead.empty()) result.comedicRead = override.comedicRead;
        addAllUnique(result.tags, override.tags);
        addAllUnique(result.moods, override.moods);
        addAllUnique(result.subjects, override.subjects);
        addAllUnique(result.aliases, override.aliases);
        addAllUnique(result.antiPatterns, override.antiPatterns);
        if (!override.textZones.empty()) result.textZones = override.textZones;
        result.tags = uniqueValues(result.tags);
        result.moods = uniqueValues(result.moods);
        result.subjects = uniqueValues(result.subjects);
        result.aliases = uniqueValues(result.aliases);
        result.antiPatterns = uniqueValues(result.antiPatterns);
        return result;
    }

    inline TemplateMetadata metadataForAsset(const std::string& id, const std::string& stem, const std::string& category) {
        auto base = fallbackAssetMetadata(stem, category);
        const auto& curated = curatedAssetMetadata();
        auto found = curated.find(id);
        if (found == curated.end()) return base;
        return mergeMetadata(base, found->second);
    }

    inline TemplateMetadata metadataForBuiltin(const TemplateDefinition& tmpl) {
        auto words = stemWords(tmpl.id);
        std::vector<std::string> tags = {"template", tmpl.category, tmpl.id};
        addAllUnique(tags, words);
        return {
            tmpl.description,
            tmpl.bestFor,
            uniqueValues(tags),
            {},
            {},
            {tmpl.id, tmpl.name},
            "Use the named slots implied by the template; prefer top/bottom captions for longer text.",
        };
    }

    inline bool isAllowedImageExt(const std::filesystem::path& path) {
        static const std::unordered_set<std::string> exts = {
            ".jpg", ".jpeg", ".png", ".gif",
        };
        return exts.count(lower(path.extension().string())) > 0;
    }

    inline json textZoneToJson(const TextZone& zone) {
        json item = {
            {"id", zone.id},
            {"label", zone.label},
            {"x", zone.x},
            {"y", zone.y},
            {"width", zone.width},
            {"height", zone.height},
            {"anchor", zone.anchor},
            {"fontSize", zone.fontSize},
            {"guidance", zone.guidance},
        };
        return item;
    }

    inline json textZonesToJson(const std::vector<TextZone>& zones) {
        json items = json::array();
        for (const auto& zone : zones) {
            items.push_back(textZoneToJson(zone));
        }
        return items;
    }

    inline json templateToJson(const TemplateEntry& entry) {
        json item = {
            {"id", entry.id},
            {"name", entry.name},
            {"category", entry.category},
            {"description", entry.description},
            {"bestFor", entry.bestFor},
            {"source", entry.source},
            {"assetPath", entry.assetPath},
        };
        if (!entry.assetCategory.empty()) {
            item["assetCategory"] = entry.assetCategory;
        }
        item["tags"] = entry.tags;
        item["moods"] = entry.moods;
        item["subjects"] = entry.subjects;
        item["aliases"] = entry.aliases;
        item["captionGuidance"] = entry.captionGuidance;
        if (!entry.comedicName.empty()) item["comedicName"] = entry.comedicName;
        if (!entry.comedicRead.empty()) item["comedicRead"] = entry.comedicRead;
        if (!entry.antiPatterns.empty()) item["antiPatterns"] = entry.antiPatterns;
        if (!entry.textZones.empty()) item["textZones"] = textZonesToJson(entry.textZones);
        return item;
    }

    inline std::vector<TemplateEntry> assetTemplateEntries(const std::string& binaryDir) {
        std::vector<TemplateEntry> out;
        auto root = std::filesystem::path(binaryDir) / "assets" / "templates";
        if (!std::filesystem::is_directory(root)) return out;

        std::vector<std::filesystem::path> categories;
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (entry.is_directory()) categories.push_back(entry.path());
        }
        std::sort(categories.begin(), categories.end());

        for (const auto& categoryPath : categories) {
            auto category = categoryPath.filename().string();
            std::vector<std::filesystem::path> images;
            for (const auto& entry : std::filesystem::directory_iterator(categoryPath)) {
                if (entry.is_regular_file() && isAllowedImageExt(entry.path())) {
                    images.push_back(entry.path());
                }
            }
            std::sort(images.begin(), images.end());

            for (const auto& image : images) {
                auto stem = image.stem().string();
                auto filename = image.filename().string();
                auto id = category + "." + stem;
                auto metadata = metadataForAsset(id, stem, category);
                auto canonicalName = titleizeStem(stem);
                addUnique(metadata.aliases, canonicalName);
                out.push_back({
                    id,
                    metadata.comedicName.empty() ? canonicalName : metadata.comedicName,
                    category,
                    metadata.description,
                    metadata.bestFor,
                    "asset",
                    category,
                    "assets/templates/" + category + "/" + filename,
                    metadata.tags,
                    metadata.moods,
                    metadata.subjects,
                    metadata.aliases,
                    metadata.captionGuidance,
                    metadata.comedicName,
                    metadata.comedicRead,
                    metadata.antiPatterns,
                    metadata.textZones,
                });
            }
        }

        return out;
    }

    inline std::vector<TemplateEntry> templateEntries(const std::string& binaryDir) {
        std::vector<TemplateEntry> out;
        for (const auto& tmpl : builtInTemplates()) {
            auto metadata = metadataForBuiltin(tmpl);
            out.push_back({
                tmpl.id,
                tmpl.name,
                tmpl.category,
                metadata.description,
                metadata.bestFor,
                "builtin",
                "",
                tmpl.assetPath,
                metadata.tags,
                metadata.moods,
                metadata.subjects,
                metadata.aliases,
                metadata.captionGuidance,
                metadata.comedicName,
                metadata.comedicRead,
                metadata.antiPatterns,
                metadata.textZones,
            });
        }
        auto assets = assetTemplateEntries(binaryDir);
        out.insert(out.end(), assets.begin(), assets.end());
        return out;
    }

    inline json templatesJson(const std::string& binaryDir) {
        json items = json::array();
        for (const auto& entry : templateEntries(binaryDir)) {
            items.push_back(templateToJson(entry));
        }
        return items;
    }

    inline json assetsJson(const std::string& binaryDir, const std::string& onlyCategory = "") {
        json grouped = json::object();
        for (const auto& entry : assetTemplateEntries(binaryDir)) {
            if (!onlyCategory.empty() && entry.assetCategory != onlyCategory) continue;
            if (!grouped.contains(entry.assetCategory)) grouped[entry.assetCategory] = json::array();
            grouped[entry.assetCategory].push_back(templateToJson(entry));
        }
        return grouped;
    }

    inline json effectsJson() {
        return json::array({
            {{"id", "none"}, {"name", "None"}, {"description", "Leave the scene clean."}, {"expression", nullptr}},
            {{"id", "sepia"}, {"name", "Sepia"}, {"description", "Warm sepia tone."}, {"expression", "sepia"}},
            {{"id", "vintage"}, {"name", "Vintage"}, {"description", "Sepia with reduced brightness."}, {"expression", "sepia >> brightness(0.9)"}},
            {{"id", "glitch"}, {"name", "Glitch"}, {"description", "Pixelation, contrast, and noise."}, {"expression", "pixelate(4) >> contrast(1.8) >> noise(0.2)"}},
            {{"id", "deepfry"}, {"name", "Deep Fry"}, {"description", "High saturation and compression chaos."}, {"expression", "saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1)"}},
            {{"id", "cyberpunk"}, {"name", "Cyberpunk"}, {"description", "Hue shift, chromatic aberration, glow."}, {"expression", "hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4)"}},
            {{"id", "comic"}, {"name", "Comic"}, {"description", "Posterize, contrast, sharpen."}, {"expression", "posterize(5) >> contrast(1.4) >> sharpen"}},
            {{"id", "retro"}, {"name", "Retro"}, {"description", "Grayscale with texture and contrast."}, {"expression", "grayscale >> contrast(1.3) >> noise(0.1)"}},
            {{"id", "grayscale"}, {"name", "Grayscale"}, {"description", "Convert to grayscale."}, {"expression", "grayscale"}},
            {{"id", "invert"}, {"name", "Invert"}, {"description", "Invert all colors."}, {"expression", "invert"}},
            {{"id", "vignette"}, {"name", "Vignette"}, {"description", "Darken the outer edges."}, {"expression", "vignette"}},
            {{"id", "sharpen"}, {"name", "Sharpen"}, {"description", "Sharpen the scene."}, {"expression", "sharpen"}},
        });
    }

    inline json effectDefinitionsJson() {
        return json::array({
            {{"id", "blur"}, {"name", "Blur"}, {"param", "radius"}, {"min", 1}, {"max", 20}, {"default", 5}, {"step", 1}},
            {{"id", "pixelate"}, {"name", "Pixelate"}, {"param", "blockSize"}, {"min", 1}, {"max", 32}, {"default", 4}, {"step", 1}},
            {{"id", "noise"}, {"name", "Noise"}, {"param", "amount"}, {"min", 0.01}, {"max", 1.0}, {"default", 0.2}, {"step", 0.05}},
            {{"id", "saturate"}, {"name", "Saturate"}, {"param", "factor"}, {"min", 0.0}, {"max", 5.0}, {"default", 1.5}, {"step", 0.1}},
            {{"id", "contrast"}, {"name", "Contrast"}, {"param", "factor"}, {"min", 0.1}, {"max", 3.0}, {"default", 1.5}, {"step", 0.1}},
            {{"id", "brightness"}, {"name", "Brightness"}, {"param", "factor"}, {"min", 0.1}, {"max", 3.0}, {"default", 1.0}, {"step", 0.1}},
            {{"id", "jpeg"}, {"name", "JPEG Artifacts"}, {"param", "quality"}, {"min", 1}, {"max", 100}, {"default", 10}, {"step", 1}},
            {{"id", "hueShift"}, {"name", "Hue Shift"}, {"param", "degrees"}, {"min", 0}, {"max", 360}, {"default", 90}, {"step", 10}},
            {{"id", "glow"}, {"name", "Glow"}, {"param", "radius"}, {"min", 1}, {"max", 20}, {"default", 4}, {"step", 1}},
            {{"id", "posterize"}, {"name", "Posterize"}, {"param", "levels"}, {"min", 2}, {"max", 16}, {"default", 5}, {"step", 1}},
            {{"id", "chromatic"}, {"name", "Chromatic"}, {"param", "offset"}, {"min", 1}, {"max", 20}, {"default", 3}, {"step", 1}},
            {{"id", "threshold"}, {"name", "Threshold"}, {"param", "level"}, {"min", 0}, {"max", 255}, {"default", 128}, {"step", 1}},
            {{"id", "sepia"}, {"name", "Sepia"}, {"param", nullptr}},
            {{"id", "invert"}, {"name", "Invert"}, {"param", nullptr}},
            {{"id", "sharpen"}, {"name", "Sharpen"}, {"param", nullptr}},
            {{"id", "vignette"}, {"name", "Vignette"}, {"param", nullptr}},
            {{"id", "grayscale"}, {"name", "Grayscale"}, {"param", nullptr}},
        });
    }

    inline json layoutsJson() {
        return json::array({
            {{"id", "single"}, {"name", "Single"}, {"slotCount", 1}, {"description", "One full-canvas scene."}},
            {{"id", "beside"}, {"name", "Beside"}, {"slotCount", 2}, {"description", "Two slots side by side."}},
            {{"id", "stack"}, {"name", "Stack"}, {"slotCount", 2}, {"description", "Two slots stacked vertically."}},
            {{"id", "grid2x2"}, {"name", "Grid 2x2"}, {"slotCount", 4}, {"description", "Four slots in a 2x2 comparison grid."}},
        });
    }

    inline json stylePresetsJson() {
        return json::array({
            {{"id", "cinematic"}, {"name", "Cinematic"}, {"description", "Soft white type with a restrained shadow."},
                {"style", {{"color", "#FFFFFF"}, {"outline", 2}, {"outlineColor", "#111111"}, {"shadow", 4}, {"shadowColor", "#00000088"}, {"fontSize", "sm"}}}},
            {{"id", "panic"}, {"name", "Panic"}, {"description", "Red alert styling with heavier edges."},
                {"style", {{"color", "#FF0000"}, {"outline", 5}, {"outlineColor", "#440000"}, {"shadow", 3}, {"shadowColor", "#00000099"}, {"fontSize", "sm"}}}},
            {{"id", "chill"}, {"name", "Chill"}, {"description", "Terminal green with crisp outline."},
                {"style", {{"color", "#00FF41"}, {"outline", 3}, {"outlineColor", "#003300"}, {"fontSize", "sm"}}}},
            {{"id", "shout"}, {"name", "Shout"}, {"description", "Loud white all-caps energy."},
                {"style", {{"color", "#FFFFFF"}, {"outline", 4}, {"outlineColor", "#000000"}, {"shadow", 3}, {"shadowColor", "#00000099"}, {"fontSize", "sm"}}}},
            {{"id", "whisper"}, {"name", "Whisper"}, {"description", "Muted grey with a fine outline."},
                {"style", {{"color", "#CCCCCC"}, {"outline", 1}, {"outlineColor", "#333333"}, {"fontSize", "sm"}}}},
        });
    }

    inline json limitsJson() {
        return {
            {"width", {{"min", 240}, {"max", 1200}}},
            {"height", {{"min", 240}, {"max", 1200}}},
            {"duration", {{"min", 80}, {"max", 2500}}},
            {"padding", {{"min", 0}, {"max", 60}}},
            {"border", {{"min", 0}, {"max", 24}}},
            {"outline", {{"min", 0}, {"max", 10}}},
            {"shadow", {{"min", 0}, {"max", 10}}},
            {"fontSizePx", {{"min", 8}, {"max", 240}}},
            {"maxScenes", 24},
            {"maxGifFrames", meme::MAX_GIF_FRAMES},
        };
    }

    inline json idList(const json& items) {
        json out = json::array();
        for (const auto& item : items) out.push_back(item.at("id"));
        return out;
    }

    inline json nativeNamesJson() {
        json out = json::array();
        for (const auto& native : native_registry::all()) {
            if (native.visibility == native_registry::NativeVisibility::Public) {
                out.push_back(native.name);
            }
        }
        return out;
    }

    inline json allowedNamesJson(const std::string& binaryDir) {
        json assets = json::object();
        auto assetGroups = assetsJson(binaryDir);
        for (auto it = assetGroups.begin(); it != assetGroups.end(); ++it) {
            assets[it.key()] = idList(it.value());
        }
        return {
            {"templates", idList(templatesJson(binaryDir))},
            {"assets", assets},
            {"effects", idList(effectsJson())},
            {"effect_definitions", idList(effectDefinitionsJson())},
            {"layouts", idList(layoutsJson())},
            {"style_presets", idList(stylePresetsJson())},
            {"natives", nativeNamesJson()},
        };
    }

    inline uint64_t fnv1a(const std::string& text) {
        uint64_t hash = 1469598103934665603ULL;
        for (unsigned char c : text) {
            hash ^= c;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    inline std::string hex64(uint64_t value) {
        std::ostringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << value;
        return ss.str();
    }

    inline json assetFingerprintStats(const std::string& binaryDir) {
        json stats = json::array();
        auto root = std::filesystem::path(binaryDir) / "assets" / "templates";
        if (!std::filesystem::is_directory(root)) return stats;

        std::vector<std::filesystem::path> images;
        for (const auto& dir : std::filesystem::directory_iterator(root)) {
            if (!dir.is_directory()) continue;
            for (const auto& entry : std::filesystem::directory_iterator(dir.path())) {
                if (entry.is_regular_file() && isAllowedImageExt(entry.path())) {
                    images.push_back(entry.path());
                }
            }
        }
        std::sort(images.begin(), images.end());

        for (const auto& image : images) {
            std::error_code relEc;
            std::error_code timeEc;
            std::error_code sizeEc;
            auto rel = std::filesystem::relative(image, std::filesystem::path(binaryDir), relEc);
            auto mtime = std::filesystem::last_write_time(image, timeEc);
            auto size = std::filesystem::file_size(image, sizeEc);
            stats.push_back({
                {"path", relEc ? image.string() : rel.generic_string()},
                {"size", sizeEc ? 0 : size},
                {"mtime", timeEc ? 0 : mtime.time_since_epoch().count()},
            });
        }
        return stats;
    }

    inline std::string catalogFingerprint(const std::string& binaryDir) {
        json payload = {
            {"mac_version", MAC_VERSION},
            {"templates", templatesJson(binaryDir)},
            {"effects", effectsJson()},
            {"effect_definitions", effectDefinitionsJson()},
            {"layouts", layoutsJson()},
            {"style_presets", stylePresetsJson()},
            {"limits", limitsJson()},
            {"allowed_names", allowedNamesJson(binaryDir)},
            {"asset_stats", assetFingerprintStats(binaryDir)},
        };
        return hex64(fnv1a(payload.dump()));
    }

    inline std::vector<std::string> splitSelectors(const std::string& selectorText) {
        std::vector<std::string> out;
        std::stringstream ss(selectorText);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char c) {
                return !std::isspace(c);
            }));
            item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char c) {
                return !std::isspace(c);
            }).base(), item.end());
            if (!item.empty()) out.push_back(item);
        }
        return out;
    }

    inline std::vector<std::string> assetCategories(const std::string& binaryDir) {
        std::set<std::string> categories;
        for (const auto& entry : assetTemplateEntries(binaryDir)) {
            categories.insert(entry.assetCategory);
        }
        return {categories.begin(), categories.end()};
    }

    inline std::vector<std::string> availableSelectors(const std::string& binaryDir) {
        std::vector<std::string> selectors = {
            "templates",
            "assets",
            "effects",
            "effect_definitions",
            "layouts",
            "style_presets",
            "limits",
            "allowed_names",
        };
        for (const auto& category : assetCategories(binaryDir)) {
            selectors.push_back("assets:" + category);
        }
        return selectors;
    }

    inline bool includeTopLevel(json& out, const std::string& binaryDir, const std::string& selector) {
        if (selector == "templates") out["templates"] = templatesJson(binaryDir);
        else if (selector == "assets") out["assets"] = assetsJson(binaryDir);
        else if (selector == "effects") out["effects"] = effectsJson();
        else if (selector == "effect_definitions") out["effect_definitions"] = effectDefinitionsJson();
        else if (selector == "layouts") out["layouts"] = layoutsJson();
        else if (selector == "style_presets") out["style_presets"] = stylePresetsJson();
        else if (selector == "limits") out["limits"] = limitsJson();
        else if (selector == "allowed_names") out["allowed_names"] = allowedNamesJson(binaryDir);
        else return false;
        return true;
    }

    inline json catalogJson(const std::string& binaryDir, const std::string& selectorText = "") {
        auto fullSelectors = std::vector<std::string>{
            "templates",
            "assets",
            "effects",
            "effect_definitions",
            "layouts",
            "style_presets",
            "limits",
            "allowed_names",
        };
        auto selectors = selectorText.empty() || selectorText == "all"
            ? fullSelectors
            : splitSelectors(selectorText);

        json out = {
            {"schema_version", 4},
            {"mac_version", MAC_VERSION},
            {"catalog_fingerprint", catalogFingerprint(binaryDir)},
            {"included", json::array()},
        };

        auto available = availableSelectors(binaryDir);
        std::unordered_set<std::string> availableSet(available.begin(), available.end());
        std::unordered_set<std::string> seen;
        for (const auto& selector : selectors) {
            if (!availableSet.count(selector)) {
                return {
                    {"ok", false},
                    {"error", "unknown_catalog_selector"},
                    {"selector", selector},
                    {"available", available},
                };
            }
            if (seen.count(selector)) continue;
            seen.insert(selector);
            out["included"].push_back(selector);

            if (selector.rfind("assets:", 0) == 0) {
                auto category = selector.substr(std::string("assets:").size());
                if (!out.contains("assets")) out["assets"] = json::object();
                out["assets"][category] = assetsJson(binaryDir, category).value(category, json::array());
            } else {
                includeTopLevel(out, binaryDir, selector);
            }
        }

        return out;
    }

    inline int printCatalog(const std::string& binaryDir, const std::string& selectorText = "") {
        auto result = catalogJson(binaryDir, selectorText);
        std::cout << result.dump() << std::endl;
        return result.value("ok", true) ? 0 : 2;
    }

} // namespace mac_catalog

#endif // MAC_CATALOG_H
