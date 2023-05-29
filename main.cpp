#include <rv/detail/definitions.hpp>
#include <rv/rv.hpp>

#include <imgui.hpp>
#include <util.hpp>

#include <stuff/bit.hpp>
#include <stuff/core.hpp>
#include <stuff/random.hpp>
#include <stuff/thread/channel.hpp>

#include <fmt/format.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <SFML/Graphics.hpp>

#include <functional>
#include <random>
#include <string_view>

struct program {
    program()
        : m_window(sf::VideoMode({1280, 720}), "lorem ipsum")
        , m_processor_worker_thread([this] { processor_worker(); }) {
        m_window.setFramerateLimit(60);
        ImGui::SFML::Init(m_window);

        load_fonts();

        m_memory_editor.Cols = 8;
        m_memory_editor.OptShowDataPreview = true;
    }

    ~program() {
        m_request_channel.close();
        m_processor_worker_thread.join();
    }

    auto main() -> int {
        for (sf::Clock frame_delta_clock{}, image_fetch_clock{}; m_window.isOpen();) {
            for (sf::Event event; m_window.pollEvent(event);) {
                ImGui::SFML::ProcessEvent(m_window, event);

                if (event.type == sf::Event::Closed)
                    m_window.close();
            }

            ImGui::SFML::Update(m_window, frame_delta_clock.restart());
            ImGui::PushFont(m_font);

            ImGui::SetNextWindowSize(ImVec2(m_window.getSize().x, m_window.getSize().y));
            ImGui::SetNextWindowPos({0, 0});
            if (ImGui::Begin("risc-v", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar)) {
                gui_menu_bar();

                ImGui::BeginTable("##main_ui_table", 3, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail());
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                {
                    auto group_guard = rv::imgui::guarded_group();

                    if (ImGui::BeginTabBar("LHSTabBar")) {
                        if (ImGui::BeginTabItem("Control and State")) {
                            ImGui::Button("Run/Pause");
                            ImGui::SameLine();
                            if (ImGui::Button("Step")) {
                                stf::send(m_request_channel, processor_request{.run = false, .amt_steps = 1});
                            }
                            rv::imgui::input_scalar("Amt Steps", m_amt_steps);
                            ImGui::SameLine();
                            if (ImGui::Button("Run For")) {
                                stf::send(m_request_channel, processor_request{.run = false, .amt_steps = m_amt_steps});
                            }

                            if (ImGui::Button("Run")) {
                                stf::send(m_request_channel, processor_request{.run = true});
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Stop")) {
                                stf::send(m_request_channel, processor_request{.run = false});
                            }

                            ImGui::SameLine();
                            ImGui::Button("Reset");

                            ImGui::BeginTable(
                              "##control_brief_info_table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_RowBg
                            );

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("PC");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(fmt::format("{:016X}", m_risc_v.program_counter()).c_str());

                            const auto next_instruction_word = m_risc_v.memory().mem_read<u32>(m_risc_v.program_counter());

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("next instruction word");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(fmt::format("{:08X}", next_instruction_word).c_str());

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("next instruction (RV32)");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(rv::isa_type_32::format(next_instruction_word).c_str());

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("next instruction (RV64)");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(rv::isa_type_64::format(next_instruction_word).c_str());

                            ImGui::EndTable();

                            ImGui::BeginTable(
                              "##state_table", 5,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_RowBg  //,
                              // ImGui::GetContentRegionAvail()
                            );

                            for (u32 i = 0; i < 16; i++) {
                                ImGui::TableNextRow();

                                const auto reg_0 = static_cast<rv::reg>(i);

                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(fmt::format("{}({})", rv::register_name<false>(reg_0), rv::register_name<true>(reg_0)).c_str());
                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(fmt::format("{:016X}", m_risc_v.read_register(reg_0)).c_str());

                                ImGui::TableNextColumn();

                                const auto reg_1 = static_cast<rv::reg>(i + 16);

                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(fmt::format("{}({})", rv::register_name<false>(reg_1), rv::register_name<true>(reg_1)).c_str());
                                ImGui::TableNextColumn();
                                ImGui::TextUnformatted(fmt::format("{:016X}", m_risc_v.read_register(reg_1)).c_str());
                            }

                            ImGui::EndTable();

                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Logs")) {
                            ImGui::Text("NYI");
                            ImGui::EndTabItem();
                        }

                        ImGui::EndTabBar();
                    }
                }

                ImGui::TableNextColumn();
                ImGui::Text("the TTY will be here");

                ImGui::TableNextColumn();
                {
                    auto group_guard = rv::imgui::guarded_group();

                    if (ImGui::BeginTabBar("RHSTabBar")) {
                        if (ImGui::BeginTabItem("Memory Editor")) {
                            m_memory_editor.DrawContents(m_risc_v.memory().data(), m_risc_v.memory().size());
                            ImGui::EndTabItem();
                        }

                        if (ImGui::BeginTabItem("Instruction Viewer")) {
                            ImGui::EndTabItem();
                        }

                        ImGui::EndTabBar();
                    }
                }
                ImGui::EndTable();
            }
            ImGui::End();

            m_window.clear();

            ImGui::PopFont();
            ImGui::SFML::Render(m_window);
            m_window.display();
        }

        return 0;
    }

private:
    sf::RenderWindow m_window;
    std::unordered_map<std::string, ImFont*> m_fonts{};
    ImFont* m_font = nullptr;
    ImFontConfig m_font_config{};

    rv::risc_v<u64, rv::isa_type_64> m_risc_v{};
    usize m_amt_steps = 0;
    MemoryEditor m_memory_editor{};

    struct processor_request {
        bool run = false;
        usize amt_steps = 0;
    };

    std::thread m_processor_worker_thread{};
    stf::channel<processor_request> m_request_channel{};

    void processor_worker() {
        for (;;) {
            auto request = processor_request{};
            if (auto recv_res = stf::receive(m_request_channel); recv_res) {
                request = *recv_res;
            } else {
                break;
            }

            constexpr auto step_batch_amt = 8uz;

            const auto batch_count = request.amt_steps / step_batch_amt;
            const auto batch_excess = request.amt_steps % step_batch_amt;

            auto step = [this](usize step_for) {
                for (usize step = 0; step < step_for; step++) {
                    const auto pc_0 = m_risc_v.m_program_counter;
                    m_risc_v.step();
                    const auto pc_1 = m_risc_v.m_program_counter;

                    if (pc_0 == pc_1) {
                        return false;
                    }
                }

                return true;
            };

            if (request.run) {
                for (bool stop = false; !stop;) {
                    if (!step(step_batch_amt)) {
                        spdlog::warn("PC didn't change (@ {:#018X}), halting", m_risc_v.m_program_counter);
                        stop = true;
                    }

                    stf::select(
                      stf::channel_selector(m_request_channel, [&stop](auto req) { stop = !req || !req->run; }),  //
                      stf::default_channel_selector([] {})
                    );
                }
            } else {
                for (usize batch_no = 0; batch_no < batch_count; batch_no++) {
                    step(step_batch_amt);
                }

                step(batch_excess);
            }
        }
    }

    void load_fonts() {
        ImGuiIO& io = ImGui::GetIO();
        m_fonts["ImGUI Default"] = io.Fonts->AddFontDefault();
        m_fonts["Proggy Clean (TTF)"] = io.Fonts->AddFontFromFileTTF("ProggyClean.ttf", 13);
        // m_fonts["Proggy Tiny (TTF)"] = io.Fonts->AddFontFromFileTTF("ProggyTiny.ttf", 13);
        m_fonts["Droid Sans (TTF)"] = io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 13);
        m_fonts["Karla Regular (TTF)"] = io.Fonts->AddFontFromFileTTF("Karla-Regular.ttf", 13);
        m_fonts["Cousine Regular (TTF)"] = io.Fonts->AddFontFromFileTTF("Cousine-Regular.ttf", 13);
        io.Fonts->Build();
        ImGui::SFML::UpdateFontTexture();

        m_font = m_fonts["Droid Sans (TTF)"];
    }

    void gui_menu_bar() {
        if (!ImGui::BeginMenuBar()) {
            return;
        }

        auto popup_about = false;
        auto popup_save_as = false;

        const auto center = ImGui::GetMainViewport()->GetCenter();

        if (ImGui::MenuItem("About")) {
            popup_about = true;
        }

        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("UI Font...")) {
                for (auto const& [name, font] : m_fonts) {
                    if (ImGui::MenuItem(name.c_str())) {
                        m_font = font;
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();

        if (popup_about) {
            ImGui::OpenPopup("About");
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        }

        if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("");
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
    }
};

int main() {
    spdlog::set_level(spdlog::level::debug);

    auto sfml_program = program{};
    return sfml_program.main();
}
