#include <windows.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <io.h>
#include <conio.h>
#include <stdio.h>

/* この実習で学んだ様々なものを使って簡単なCUIのゲームを作ってみよう。
 * 作成するものは何でもよいが下記の要件を満たす事。
 * タイトル ゲームシーケンス 結果表示が存在しそれらがステートマシンによってコントロールされていること（任意)
 * クラスの多態性を用いたオブジェクト管理が入っていること(任意)
 * 自分が作成した Poolアロケータが使われていること(必須)
 * 何かしらの処理にスレッドを用いた並行処理が入っていること(任意)
 * ゲームエンジンの使用は不可
 * 使用する言語は C ++
 */

#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <set>
#ifdef max
#undef max
#endif
#include <windows.h>
#include <limits> 
#include "PoolAllocator.h"

 // === ゲーム設定 ===
struct StageConfig {
    std::string title;
    std::string description;
    std::vector<std::string> intro_dialogues;
    int digits; // 桁数
    int num_range;  // 数字の範囲（0~9）
    int max_attempts; // 最大試行回数
    int score_on_clear; // クリア時のスコア
};



// === ゲームの状態の管理 ===
enum class GameState { Title, StageIntro, Playing, StageClear, GameOver, Result, Exit };

// === ヒット＆ブローの試行結果 ===
struct Attempt {
    std::string guess;
    int hit;
    int blow;
};

// === ゲーム状態の管理クラス ===
class GameStateManager {
public:
    static GameStateManager& getInstance() {
        static GameStateManager instance;
        return instance;
    }

    // コピーと代入を禁止
    GameStateManager(const GameStateManager&) = delete;
    GameStateManager& operator=(const GameStateManager&) = delete;

    void initialize() {
        stages.push_back({ "Mission 1:ハッカー入門試験", "3桁(0-5)のロックを解除せよ！",
            { //Mission 1　のセリフ
                "司令官:「よく来たな、ルーキー。」",
                "司令官:「状況は最悪だ。我々は今、窮地に立たされている。」",
                "司令官:「正体不明の組織にNOCリストを奪われた。」",
                "自分:  「NOCリスト...?」",
                "司令官:「NOC-Non Official Cover。民間人を装って他国や組織に潜入している諜報員や工作員だ。」",
                "司令官:「NOCリストは、世界中のスパイの情報が記されている。」",
                "司令官:「もし漏洩すれば、世界の諜報網は一夜で崩壊するだろう...。」",
                "司令官:「君の任務はただひとつ...、リストを奪還することだ。」",
                "司令官:「だがその前に基本的なハッキング技術を身につける必要がある。」",
                "司令官:「まずはハッキング試験だ。これを突破すれば次のフェーズに進める。」",
                "司令官:「ミッション内容を送る。準備はいいな？」",
            }, 3, 5, 15, 100 });
        stages.push_back({ "Mission 2:潜入開始！", "4桁(0-5)のロックを解除せよ！",
            { //Mission 2　のセリフ
                "司令官:「試験は...合格だ。」",
                "司令官:「これより本任務に移行する。目標はNOCリストの奪還だ。」",
                "司令官:「敵のセキュリティは強力だが、君なら突破できるはずだ。」",
                "司令官:「敵の施設に潜入し、第一防衛ラインを破れ！」",
            }, 4, 5, 15, 200 });
        stages.push_back({ "Mission 3:立ちはだかるファイアウォール", "4桁(0-5)のロックを10手以内に解除せよ！",
            {  // Mission 3 のセリフ
                "自分:「奪われたNOCリストまで、距離20！」",
                "司令官:「順調だな。だが、ここから先が地獄だぞ。」",
                "オペレーター:「司令官！強力なファイアウォールを検知！試行回数が10回に制限されます！」",
                "司令官:「なにぃ！？...ルーキー、聞こえるな？一手一手慎重にやれ。」",
            }, 4, 5, 10, 300 });
        stages.push_back({ "Mission 4:最終決戦！", "敵ハッカーより先に4桁(0-9)のロックを10手以内に解除せよ！",
            { // Mission 4 のセリフ
                "オペレーター:「ターゲットのメインフレームに到達！」",
                "オペレーター:「ですが、敵ハッカーも同時に侵入してきました！」",
                "司令官:「迷うな、ルーキー！指はキーボードから離すな！急げ！」",
            }, 4, 9, 10, 500 });
        secretStage = { "隠しMission:バ○ダイナムコスタジオからの刺客", "6桁(0-9)のロックを15手以内に解除せよ!",
            { // 隠しステージのセリフ
                "自分:「あ、あなたは!? 」",
                "？？？:「よくぞここまで来たな、挑戦者よ。」",
                "？？？:「我々からの『挑戦状』、受けてみるがいい...！」",
            }, 6, 9, 15, 600 };
    }
    //　読み取り専用
    const StageConfig& getStage(int index) const {
        if (index < 0 || index >= static_cast<int>(stages.size())) {
            throw std::out_of_range("Invalid stage index");
        }
        return stages[index];
    }

    const StageConfig& getCurrentStage() const {
        if (isSecretStageMode) {
            return secretStage;
        }
        return getStage(currentStageIndex);
    }

    // 次のステージに進むための関数
    void ToNextStage() {
        if (currentStageIndex < static_cast<int>(stages.size()) - 1) {
            currentStageIndex++;
        }
        else {
            allStagesCleared = true;
        }
    }

    int getCurrentStageIndex() const {
        return currentStageIndex;
    }
    bool isAllStagesCleared() const {
        return allStagesCleared;
    }
    bool canUseSum() const {
        return !Used_Sum;
    }
    bool canUseScan() const {
        return !Used_Scan;
    }
    bool canUseLeak() const {
        return !Used_Leak;
    }
    bool getSecretStageMode() const {
        return isSecretStageMode;
    }
    int getStageCount() const { return static_cast<int>(stages.size()); }

    void useSum() { Used_Sum = true; }
    void useScan() { Used_Scan = true; }
    void useLeak() { Used_Leak = true; }
    void setSecretStageMode(bool mode) { isSecretStageMode = mode; }
    void resetGame() {
        currentStageIndex = 0;
        allStagesCleared = false;
        Used_Sum = false;
        Used_Scan = false;
        Used_Leak = false;
        isSecretStageMode = false;
    }
private:
    GameStateManager() { initialize(); resetGame(); }

    std::vector<StageConfig> stages;
    StageConfig secretStage;
    int currentStageIndex;
    bool allStagesCleared;
    bool Used_Sum;
    bool Used_Scan;
    bool Used_Leak;
    bool isSecretStageMode;
};


// === シーンの設定 ===
class Scene {
public:
    virtual void update(GameState& state) = 0;
    virtual ~Scene() {}
};

// === タイトルシーン ===
class TitleScene : public Scene {
public:
    void update(GameState& state) override {
        system("cls");
        std::cout << "========================\n";
        std::cout << "   Blow the Cover \n";
        std::cout << "========================\n\n";
        std::cout << "あなたは新米ハッカー。\n";
        std::cout << "敵にNOCリストが奪われた!全力で奪還せよ。\n\n";
        std::cout << "[Enter]でゲーム開始...\n";
        std::cout << "[何かキー]を押してからEnterで終了...\n\n";
        std::cout << "隠しコマンドがあるらしい...?(コナミコマンドっていいよね)\n";

        // Konami Codeの入力を待つ
        const std::vector<int> konamiCode = { 72, 72, 80, 80, 75, 77, 75, 77, 'b', 'a' };
        std::vector<int> inputSequence;
        std::string input_str;

        while (true) {
            // _getch()はconio.hで定義されているキーボードから1文字入力する関数
            int ch = _getch();
            // Enterキーが押されたらループを抜ける
            if (ch == '\r') { // Enter key
                if (input_str.length() == 0) {
                    state = GameState::StageIntro;
                }
                else {
                    state = GameState::Exit;
                }
                break;
            }

            if (ch == 224 || ch == 0) {
                inputSequence.push_back(_getch());
            }
            else {
                inputSequence.push_back(ch);
                input_str += static_cast<char>(ch);
            }

            // 入力シーケンスの確認　コナミコマンドと一致するか
            if (inputSequence.size() == konamiCode.size()) {
                if (inputSequence == konamiCode) {
                    GameStateManager::getInstance().setSecretStageMode(true);
                    std::cout << "\n\n*** 隠しステージが出現した！ ***\n";
                    Sleep(1500);
                    state = GameState::StageIntro;
                    break;
                }
                inputSequence.erase(inputSequence.begin());
            }
        }
    }
};

// === ユーティリティ関数 ===
void typeWriterPrint(const std::string& text, unsigned int delay_ms) {
    for (char c : text) {
        std::cout << c << std::flush;
        Sleep(delay_ms);
    }
    std::cout << std::endl;
}

// === ステージ開始シーン ===
class StageIntroScene : public Scene {
public:
    void update(GameState& state) override {
        system("cls");
        const auto& config = GameStateManager::getInstance().getCurrentStage();
        std::cout << "------------------------------------------\n";
        std::cout << "=== " << config.title << " ===\n";
        std::cout << "------------------------------------------\n\n";

        // --- セリフパート ---
        for (const auto& line : config.intro_dialogues) {
            typeWriterPrint(line, 20); // 50ミリ秒待機
            Sleep(500); // 次のセリフまでの間
        }

        std::cout << "目的: " << config.description << "\n\n";
        std::cout << "[Enter]で挑戦開始...\n";

        // cinのバッファをクリア
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::cin.get();
        state = GameState::Playing;
    }
};

// === プレイシーン ===
class PlayScene : public Scene {
    const StageConfig& config;
    std::string answer;
    PoolAllocator<Attempt, 20> pool;
    std::vector<Attempt*> attempts;
    bool isTutorial;

    std::string generateAnswer() {
        std::string digits = "0123456789";
        // 0~config.num_rangeの数字を使用して、config.digits桁の数字を生成
        std::string source_digits = digits.substr(0, config.num_range + 1);
        std::shuffle(source_digits.begin(), source_digits.end(), std::mt19937(std::random_device{}()));
        return source_digits.substr(0, config.digits);
    }

    Attempt evaluate(const std::string& guess) {
        Attempt a{ guess, 0, 0 };
        for (int i = 0; i < config.digits; ++i) {
            if (guess[i] == answer[i]) {
                a.hit++;
            }
            else if (answer.find(guess[i]) != std::string::npos) {
                a.blow++;
            }
        }
        return a;
    }

    void executeSkill(const std::string& skill) {
        auto& gm = GameStateManager::getInstance();
        if (skill == "leak" && gm.canUseLeak()) {
            std::cout << "\n*** SKILL: LEAK 発動！ ***\n";
            std::cout << "オペレーター:「正解に含まれる数字は... 「" << answer[rand() % config.digits] << "」だ。」\n";
            gm.useLeak();
        }
        else if (skill == "sum" && gm.canUseSum()) {
            int sum = 0;
            for (char c : answer) { sum += c - '0'; }
            std::cout << "\n*** SKILL: SUM 発動！ ***\n";
            std::cout << "オペレーター:「正解の数字の合計値は... 「" << sum << "」だ。」\n";
            gm.useSum();
        }
        else if (skill == "scan" && gm.canUseScan()) {
            std::cout << "\n*** SKILL: SCAN 発動！ ***\n";
            int pos;
            std::cout << "スキャンする桁を入力してください (1-" << config.digits << "): ";
            std::cin >> pos;
            if (std::cin.fail() || pos < 1 || pos > config.digits) {
                std::cout << "オペレーター:「その桁は無効だ。落ち着いてタイピングしよう。」(Enter押して入力に戻る)\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            else {
                std::cout << "オペレーター:「指定された" << pos << "桁目の数字は... 「" << answer[pos - 1] << "」だ。」\n";
                gm.useScan();
            }
        }
        else {
            std::cout << "\nオペレーター:「そのスキルは存在しないか、既に使用済みだ」。\n";
        }
        std::cout << "[Enter]で続行...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }


public:
    PlayScene(const StageConfig& cfg) : config(cfg) {
        answer = generateAnswer();
        // std::cout << "DEBUG: " << answer << "\n"; // デバッグ用
        // ステージインデックスが0、かつ隠しステージでないならチュートリアルと判定
        isTutorial = (GameStateManager::getInstance().getCurrentStageIndex() == 0 && !GameStateManager::getInstance().getSecretStageMode());
    }
    ~PlayScene() {
        for (auto a : attempts) {
            pool.Free(a);
        }
    }

    void update(GameState& state) override {
        if (isTutorial) {
            system("cls");
            std::vector<std::string> tutorial_lines = {
                "司令官:「チュートリアルを開始する。」",
                "司令官:「君の目的は、隠された正しい数字の並びを当てることだ。」",
                "司令官:「入力した数字に対するヒントは2種類、『Hit』と『Blow』で与えられる。」",
                "",
                "【Hit】 : 数字も位置も合っている数。",
                "【Blow】: 数字は合っているが、位置が違う数。",
                "",
                "司令官:「これを元に、正解を推理するんだ。」",
                "司令官:「さらに、ハッキングを補助する強力な『スキル』が3種類、各1回だけ使用できる。」",
                "",
                "【leak】: 正解の数字のうち、1つをランダムに教えてくれる。",
                "【sum】 : 正解の数字をすべて足した合計値を教えてくれる。",
                "【scan】: 指定した『桁』の数字が何であるかを正確に教えてくれる。",
                "",
                "司令官:「スキルはコマンドとして入力することで発動する。使いどころが重要だぞ。」",
                "司令官:「説明は以上だ。幸運を祈る。」"
            };
            for (const auto& line : tutorial_lines) {
                typeWriterPrint(line, 30);
                if (!line.empty()) Sleep(100);
            }

            std::cout << "\n[Enter]でミッション開始...";
            std::cin.get();
        }
        std::string input;
        while (true) {
            system("cls");
            std::cout << "--- " << config.title << " ---\n";
            std::cout << "残り試行回数: " << (config.max_attempts - attempts.size()) << "\n\n";
            // 履歴表示
            for (const auto& a : attempts) {
                std::cout << a->guess << " → Hit: " << a->hit << ", Blow: " << a->blow << "\n";
            };


            std::cout << "\n--- 操作 -------------------------------\n";
            std::cout << "目的: " << config.description << "\n\n";

            std::cout << config.digits << "桁、範囲内の数字を入力してください (重複なし)。\n\n";
            std::cout << "スキル (ゲーム中各1回のみ):\n";
            std::cout << "[leak]: 正解の数字のうち、1つをランダムに教えてくれる。\n";
            std::cout << "[sum] : 正解の数字をすべて足した合計値を教えてくれる。\n";
            std::cout << "[scan]: 指定した桁の数字を正確に教えてくれる。\n\n";
            std::cout << "[q]でゲームを終了します。\n";
            std::cout << "----------------------------------------\n";
            std::cout << attempts.size() + 1 << " 回目の入力: ";
            std::cin >> input;

            if (input == "q") {
                state = GameState::Exit;
                return;
            }

            // スキル処理
            if (input == "leak" || input == "sum" || input == "scan") {
                executeSkill(input);
                continue;
            }

            // 入力チェック
            if (static_cast<int>(input.length()) != config.digits || !std::all_of(input.begin(), input.end(), ::isdigit)) {
                std::cout << "オペレーター:「無効な入力だ。」 (" << config.digits << "桁の数字ではありません)(Enter押して入力に戻る)\n";
                std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                std::cin.get();
                continue;
            }


            // 数字の範囲チェック
            bool validRange = true;
            for (char c : input) {
                if (static_cast<int>(c - '0') > config.num_range) {
                    validRange = false;
                    break;
                }
            }
            if (!validRange) {
                std::cout << "オペレーター:「数字の範囲が無効だ。」(0-" << config.num_range << "の数字を使用してください)(Enter押して入力に戻る)\n";
                std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                std::cin.get();
                continue;
            }

            // 重複チェック
            std::set<char> unique(input.begin(), input.end());
            if (static_cast<int>(unique.size()) != config.digits) {
                std::cout << "オペレーター:「数字が重複しているぞ！」(Enter押して入力に戻る)\n";
                std::cin.get();
                std::cin.get();
                continue;
            }

            // 判定処理
            Attempt result = evaluate(input);

            // PoolAllocatorからメモリ確保
            Attempt* a = pool.Alloc();
            if (a) {
                *a = result;
                attempts.push_back(a);
            }
            else {
                std::cout << "メモリ不足！\n";
                state = GameState::Result;
                return;
            }

            // クリア判定
            if (result.hit == config.digits) {
                std::cout << "\n*** LOCK OPEN! ***\n";
                std::cout << "\n司令官:「成功だ！でかしたぞ！正解は " << answer << " だ。」\n";
                std::cout << "[Enter]で次へ...\n";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
                state = GameState::StageClear;
                return;
            }


            // ゲームオーバー判定
            if (static_cast<int>(attempts.size()) >= config.max_attempts) {
                system("cls");
                std::cout << "\n--- MISSION FAILED ---\n";
                std::cout << "司令官:「時間かかりすぎだ...。正解は " << answer << " だったぞ。」\n";
                std::cout << "司令官:「悔しさは次に活かせ。……必ず奪い返すぞ。」\n\n";
                std::cout << "[Enter]で次へ...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
                state = GameState::GameOver;
                return;
            }
        }
    }
};

// === 結果シーン ===
class ResultScene : public Scene {
public:
    void update(GameState& state) override {
        system("cls");
        auto& gm = GameStateManager::getInstance();
        std::cout << "========================\n";
        std::cout << "       リザルト\n";
        std::cout << "========================\n\n";

        if (gm.getSecretStageMode()) {
            if (gm.isAllStagesCleared()) {
                std::cout << "隠しステージを見事クリア！\n";
            }
            else {
                std::cout << "隠しステージクリアならず...\n";
            }
        }
        else {
            int cleared_stages = gm.getCurrentStageIndex();
            if (gm.isAllStagesCleared()) {
                cleared_stages++;
            }
            std::cout << "クリアステージ: " << cleared_stages << " / " << gm.getStageCount() << "\n\n";
            if (gm.isAllStagesCleared()) {
                std::cout << "司令官:「……任務完了だ。NOCリスト、完全奪還！」\n";
                std::cout << "司令官:「よくやった、エージェント。君は世界を救ったんだ。」\n";
                std::cout << "オペレーター:「すごい!本当にすごいよ！君はもう、伝説のハッカーだ！\n";
                std::cout << "司令官:「休め、英雄。だが――次の戦場は、すぐにやってくる。」\n\n";

            }
        }

        if (!gm.canUseLeak()) std::cout << "[SKILL: LEAK] 使用済み\n";
        if (!gm.canUseSum()) std::cout << "[SKILL: SUM] 使用済み\n";
        if (!gm.canUseScan()) std::cout << "[SKILL: SCAN] 使用済み\n";

        std::cout << "\n[r]でもう一度挑戦 / [q]で終了\n";
        char choice;
        std::cin >> choice;
        if (choice == 'r') {
            GameStateManager::getInstance().resetGame();
            state = GameState::Title; // タイトルに戻る
        }
        else state = GameState::Exit;
    }
};

// === メイン関数 ===
int main() {
    // エスケープシーケンスの有効化（Windows専用）
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdOut, &mode);
    SetConsoleMode(hStdOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    printf("\x1b[?25l"); // カーソル非表示

    GameState state = GameState::Title;
    Scene* currentScene = nullptr;

    while (state != GameState::Exit) {
        // 現在のシーンを解放
        delete currentScene;
        currentScene = nullptr;

        // ゲーム状態に応じてシーンを切り替え
        switch (state) {
        case GameState::Title:
            currentScene = new TitleScene();
            break;
        case GameState::StageIntro:
            currentScene = new StageIntroScene();
            break;
        case GameState::Playing:
            currentScene = new PlayScene(GameStateManager::getInstance().getCurrentStage());
            break;
        case GameState::StageClear:
        { // case内で変数宣言するためスコープを追加
            auto& gm = GameStateManager::getInstance();

            if (gm.getSecretStageMode()) {
                gm.ToNextStage();
                state = GameState::Result;
                continue;
            }

            // 現在のステージが最終ステージかどうかをインデックスで直接判定する
            bool isLastStage = (gm.getCurrentStageIndex() >= gm.getStageCount() - 1);

            gm.ToNextStage(); // ステージ情報を更新

            if (isLastStage) {
                state = GameState::Result;      // 最終ステージクリア後はリザルトへ
            }
            else {
                state = GameState::StageIntro;  // それ以外は次のステージへ
            }
            continue;
        }

        case GameState::GameOver:
            state = GameState::Result;
            continue;
        case GameState::Result:
            currentScene = new ResultScene();
            break;
        default:
            state = GameState::Exit;
            break;
        }
        if (currentScene) currentScene->update(state);
    }

    delete currentScene;

    // カーソルを表示
    printf("\x1b[?25h");
    return EXIT_SUCCESS;
}
