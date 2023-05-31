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

#include <chrono>
#include <functional>
#include <random>
#include <string_view>

template<std::floating_point T>
struct averager {
    constexpr averager(usize window_size)
        : m_data(window_size) {}

    constexpr auto average() const -> T {
        if (!m_average_invalid) {
            return m_average_cache;
        }

        m_average_invalid = false;
        const auto avg = std::reduce(m_data.begin(), m_data.end(), T(0)) / static_cast<T>(m_data.size());
        return m_average_cache = avg;
    }

    constexpr auto stdev() const -> T {
        if (!m_stdev_invalid) {
            return m_stdev_cache;
        }

        m_stdev_invalid = false;

        const auto sum = std::accumulate(m_data.begin(), m_data.end(), T(0), [this](T accumulator, T val) {
            const auto temp = val - average();
            return accumulator + temp * temp;
        });

        return m_stdev_cache = std::sqrt(sum / static_cast<T>(m_data.size()));
    }

    constexpr auto below_z_score(T z) const -> T {
        const auto at_most = average() + stdev() * z;

        usize count = 0;
        const auto sum = std::accumulate(m_data.begin(), m_data.end(), T(0), [at_most, &count](T accumulator, T val) {
            if (val > at_most) {
                return accumulator;
            }

            ++count;
            return accumulator + val;
        });

        if (count == 0) {
            return 0;
        }

        return sum / static_cast<T>(count);
    }

    constexpr auto percentile_95() const -> T { return below_z_score((T)-1.6449); }

    constexpr auto percentile_99() const -> T { return below_z_score((T)-2.3263); }

    constexpr auto add_sample(T v) {
        m_average_invalid = true;
        m_stdev_invalid = true;

        m_data[m_data_ptr++ % m_data.size()] = v;
    }

private:
    std::vector<T> m_data;

    mutable bool m_average_invalid = true;
    mutable T m_average_cache;

    mutable bool m_stdev_invalid = true;
    mutable T m_stdev_cache;

    usize m_data_ptr = 0;
};

struct program {
    program()
        : m_window(sf::VideoMode({1280, 720}), "lorem ipsum")
        , m_processor_worker_thread([this] { processor_worker(); }) {
        m_risc_v.load("a.hex", rv::infmt_ihex_tag{}, 0);

        m_window.setFramerateLimit(60);
        std::ignore = ImGui::SFML::Init(m_window);

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
                            if (ImGui::Button("Run")) {
                                stf::send(m_request_channel, processor_request{.run = true});
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Stop")) {
                                stf::send(m_request_channel, processor_request{.run = false});
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Step")) {
                                stf::send(m_request_channel, processor_request{.run = false, .amt_steps = 1});
                            }
                            ImGui::SameLine();
                            ImGui::Button("Reset");

                            rv::imgui::input_scalar("Amt Steps", m_amt_steps);
                            ImGui::SameLine();
                            if (ImGui::Button("Run For")) {
                                stf::send(m_request_channel, processor_request{.run = false, .amt_steps = m_amt_steps});
                            }

                            ImGui::TextUnformatted(fmt::format("Instructions per Second: {:.2f}", m_ips_averager.average()).c_str());
                            ImGui::TextUnformatted(fmt::format("Instructions per Second (95th): {:.2f}", m_ips_averager.percentile_95()).c_str());
                            ImGui::TextUnformatted(fmt::format("Instructions per Second (99th): {:.2f}", m_ips_averager.percentile_99()).c_str());

                            ImGui::BeginTable(
                              "##control_brief_info_table", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_RowBg
                            );

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("PC");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(fmt::format("{:016X}", m_risc_v.program_counter()).c_str());

                            const auto next_instruction_word = m_risc_v.memory().read<u32>(m_risc_v.program_counter());

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("next instruction word");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(fmt::format("{:08X}", next_instruction_word).c_str());

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("next instruction (RV32)");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(rv::is_rv64i<rv::risc_v<u64>>.format(next_instruction_word).c_str());

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted("next instruction (RV64)");
                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(rv::is_rv64i<rv::risc_v<u64>>.format(next_instruction_word).c_str());

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

    averager<double> m_ips_averager{4096};
    rv::risc_v<u64> m_risc_v{rv::is_rv64i<rv::risc_v<u64>>, 0x1'0000, {}};
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

            constexpr auto step_batch_amt = 32uz;

            const auto batch_count = request.amt_steps / step_batch_amt;
            const auto batch_excess = request.amt_steps % step_batch_amt;

            auto step = [this](usize step_for) {
                const auto tp_0 = std::chrono::system_clock::now();
                stf::scope::scope_exit chrono_guard{[this, tp_0] {
                    const auto tp_1 = std::chrono::system_clock::now();

                    const auto t_delta_secs = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(tp_1 - tp_0).count()) / 1e6;

                    m_ips_averager.add_sample(static_cast<double>(step_batch_amt) / t_delta_secs);
                }};

                for (usize step = 0; step < step_for; step++) {
                    const auto pc_0 = m_risc_v.m_program_counter;
                    m_risc_v.step();
                    const auto pc_1 = m_risc_v.m_program_counter;

                    if (pc_0 == pc_1) [[unlikely]] {
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
        std::ignore = ImGui::SFML::UpdateFontTexture();

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
