#ifdef _WIN32
    #include <Windows.h>
    #include <shellapi.h>
#endif
#include <browedit/BrowEdit.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>

#include <imgui.h>
#include <imgui_markdown.h>

#include <map>

static std::string markdown_ = "";
static std::vector<std::string> history;

void gotoPage(const std::string& f, bool recordHistory = true)
{
    if (f.substr(0, 4) == "http")
    {
        ShellExecuteA(NULL, "open", f.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else
    {
        auto file = util::FileIO::open("docs\\" + f);
        if (file)
        {
            markdown_ = std::string(std::istreambuf_iterator<char>(*file), std::istreambuf_iterator<char>());
            markdown_ = "![banner](banner.png)\n" + markdown_;

            auto i = 3;
            int commentStart = -1;
            while (i < markdown_.size())
            {
                if (markdown_.substr(i - 3, 4) == "<!--")
                    commentStart = i-3;
                if (markdown_.substr(i - 2, 3) == "-->")
                {
                    markdown_ = markdown_.substr(0, commentStart) + markdown_.substr(i+1);
                    i = commentStart;
                    commentStart = -1;
                }
                i++;
            }


            delete file;
            if (recordHistory)
                history.push_back(f);
        }
    }
}

void LinkCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);
    gotoPage(url);
}


std::map<std::string, gl::Texture*> textures;
inline ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data_)
{
    std::string url(data_.link, data_.linkLength);


    gl::Texture* texture = textures[url];
    if (!texture)
    {
        texture = util::ResourceManager<gl::Texture>::load("docs\\" + url);
        textures[url] = texture;
    }

    ImGui::MarkdownImageData imageData;
    imageData.isValid = true;
    imageData.useLinkCallback = false;
    imageData.user_texture_id = (ImTextureID)(long long)texture->id();
    imageData.size = ImVec2((float)texture->width, (float)texture->height);

    // For image resize when available size.x > image width, add
    ImVec2 const contentSize = ImGui::GetContentRegionAvail();
    if (imageData.size.x > contentSize.x)
    {
        float const ratio = imageData.size.y / imageData.size.x;
        imageData.size.x = contentSize.x;
        imageData.size.y = contentSize.x * ratio;
    }

    return imageData;
}


void FormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_)
{
    // Call the default first so any settings can be overwritten by our implementation.
    // Alternatively could be called or not called in a switch statement on a case by case basis.
    // See defaultMarkdownFormatCallback definition for furhter examples of how to use it.
    ImGui::defaultMarkdownFormatCallback(markdownFormatInfo_, start_);

    switch (markdownFormatInfo_.type)
    {
    case ImGui::MarkdownFormatType::HEADING:
    {
        //if (markdownFormatInfo_.level == 1)
        {
            if (start_)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            }
            else
            {
                ImGui::PopStyleColor();
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void BrowEdit::showHelpWindow()
{
    if (history.size() == 0)
    {
        gotoPage("Readme.md");
    }

	ImGui::Begin("Help", &windowData.helpWindowVisible);

    if (ImGui::Button("Back"))
    {
        if (history.size() > 1)
        {
            history.pop_back();
            gotoPage(history.back(), false);
        }
    }


    static ImGui::MarkdownConfig mdConfig;
    mdConfig.linkCallback = LinkCallback;
    mdConfig.tooltipCallback = NULL;
    mdConfig.imageCallback = ImageCallback;
    mdConfig.formatCallback = FormatCallback;
//    mdConfig.linkIcon = ICON_FA_LINK;
//    mdConfig.headingFormats[0] = { H1, true };
//    mdConfig.headingFormats[1] = { H2, true };
    mdConfig.userData = NULL;
//    mdConfig.formatCallback = ExampleMarkdownFormatCallback;
    ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);


	ImGui::End();
}