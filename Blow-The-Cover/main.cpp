#include <windows.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <io.h>
#include <conio.h>
#include <stdio.h>

/* ���̎��K�Ŋw�񂾗l�X�Ȃ��̂��g���ĊȒP��CUI�̃Q�[��������Ă݂悤�B
 * �쐬������͉̂��ł��悢�����L�̗v���𖞂������B
 * �^�C�g�� �Q�[���V�[�P���X ���ʕ\�������݂�����炪�X�e�[�g�}�V���ɂ���ăR���g���[������Ă��邱�Ɓi�C��)
 * �N���X�̑��Ԑ���p�����I�u�W�F�N�g�Ǘ��������Ă��邱��(�C��)
 * �������쐬���� Pool�A���P�[�^���g���Ă��邱��(�K�{)
 * ��������̏����ɃX���b�h��p�������s�����������Ă��邱��(�C��)
 * �Q�[���G���W���̎g�p�͕s��
 * �g�p���錾��� C ++
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

 // === �Q�[���ݒ� ===
struct StageConfig {
    std::string title;
    std::string description;
    std::vector<std::string> intro_dialogues;
    int digits; // ����
    int num_range;  // �����͈̔́i0~9�j
    int max_attempts; // �ő厎�s��
    int score_on_clear; // �N���A���̃X�R�A
};



// === �Q�[���̏�Ԃ̊Ǘ� ===
enum class GameState { Title, StageIntro, Playing, StageClear, GameOver, Result, Exit };

// === �q�b�g���u���[�̎��s���� ===
struct Attempt {
    std::string guess;
    int hit;
    int blow;
};

// === �Q�[����Ԃ̊Ǘ��N���X ===
class GameStateManager {
public:
    static GameStateManager& getInstance() {
        static GameStateManager instance;
        return instance;
    }

    // �R�s�[�Ƒ�����֎~
    GameStateManager(const GameStateManager&) = delete;
    GameStateManager& operator=(const GameStateManager&) = delete;

    void initialize() {
        stages.push_back({ "Mission 1:�n�b�J�[���厎��", "3��(0-5)�̃��b�N����������I",
            { //Mission 1�@�̃Z���t
                "�i�ߊ�:�u�悭�����ȁA���[�L�[�B�v",
                "�i�ߊ�:�u�󋵂͍ň����B��X�͍��A���n�ɗ�������Ă���B�v",
                "�i�ߊ�:�u���̕s���̑g�D��NOC���X�g��D��ꂽ�B�v",
                "����:  �uNOC���X�g...?�v",
                "�i�ߊ�:�uNOC-Non Official Cover�B���Ԑl�𑕂��đ�����g�D�ɐ������Ă��钳�����H������B�v",
                "�i�ߊ�:�uNOC���X�g�́A���E���̃X�p�C�̏�񂪋L����Ă���B�v",
                "�i�ߊ�:�u�����R�k����΁A���E�̒���Ԃ͈��ŕ��󂷂邾�낤...�B�v",
                "�i�ߊ�:�u�N�̔C���͂����ЂƂ�...�A���X�g��D�҂��邱�Ƃ��B�v",
                "�i�ߊ�:�u�������̑O�Ɋ�{�I�ȃn�b�L���O�Z�p��g�ɂ���K�v������B�v",
                "�i�ߊ�:�u�܂��̓n�b�L���O�������B�����˔j����Ύ��̃t�F�[�Y�ɐi�߂�B�v",
                "�i�ߊ�:�u�~�b�V�������e�𑗂�B�����͂����ȁH�v",
            }, 3, 5, 15, 100 });
        stages.push_back({ "Mission 2:�����J�n�I", "4��(0-5)�̃��b�N����������I",
            { //Mission 2�@�̃Z���t
                "�i�ߊ�:�u������...���i���B�v",
                "�i�ߊ�:�u������{�C���Ɉڍs����B�ڕW��NOC���X�g�̒D�҂��B�v",
                "�i�ߊ�:�u�G�̃Z�L�����e�B�͋��͂����A�N�Ȃ�˔j�ł���͂����B�v",
                "�i�ߊ�:�u�G�̎{�݂ɐ������A���h�q���C����j��I�v",
            }, 4, 5, 15, 200 });
        stages.push_back({ "Mission 3:�����͂�����t�@�C�A�E�H�[��", "4��(0-5)�̃��b�N��10��ȓ��ɉ�������I",
            {  // Mission 3 �̃Z���t
                "����:�u�D��ꂽNOC���X�g�܂ŁA����20�I�v",
                "�i�ߊ�:�u�������ȁB�����A��������悪�n�������B�v",
                "�I�y���[�^�[:�u�i�ߊ��I���͂ȃt�@�C�A�E�H�[�������m�I���s�񐔂�10��ɐ�������܂��I�v",
                "�i�ߊ�:�u�Ȃɂ��I�H...���[�L�[�A��������ȁH�����T�d�ɂ��B�v",
            }, 4, 5, 10, 300 });
        stages.push_back({ "Mission 4:�ŏI����I", "�G�n�b�J�[�����4��(0-9)�̃��b�N��10��ȓ��ɉ�������I",
            { // Mission 4 �̃Z���t
                "�I�y���[�^�[:�u�^�[�Q�b�g�̃��C���t���[���ɓ��B�I�v",
                "�I�y���[�^�[:�u�ł����A�G�n�b�J�[�������ɐN�����Ă��܂����I�v",
                "�i�ߊ�:�u�����ȁA���[�L�[�I�w�̓L�[�{�[�h���痣���ȁI�}���I�v",
            }, 4, 9, 10, 500 });
        secretStage = { "�B��Mission:�o���_�C�i���R�X�^�W�I����̎h�q", "6��(0-9)�̃��b�N��15��ȓ��ɉ�������!",
            { // �B���X�e�[�W�̃Z���t
                "����:�u���A���Ȃ���!? �v",
                "�H�H�H:�u�悭�������܂ŗ����ȁA����҂�B�v",
                "�H�H�H:�u��X����́w�����x�A�󂯂Ă݂邪����...�I�v",
            }, 6, 9, 15, 600 };
    }
    //�@�ǂݎ���p
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

    // ���̃X�e�[�W�ɐi�ނ��߂̊֐�
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


// === �V�[���̐ݒ� ===
class Scene {
public:
    virtual void update(GameState& state) = 0;
    virtual ~Scene() {}
};

// === �^�C�g���V�[�� ===
class TitleScene : public Scene {
public:
    void update(GameState& state) override {
        system("cls");
        std::cout << "========================\n";
        std::cout << "   Blow the Cover \n";
        std::cout << "========================\n\n";
        std::cout << "���Ȃ��͐V�ăn�b�J�[�B\n";
        std::cout << "�G��NOC���X�g���D��ꂽ!�S�͂ŒD�҂���B\n\n";
        std::cout << "[Enter]�ŃQ�[���J�n...\n";
        std::cout << "[�����L�[]�������Ă���Enter�ŏI��...\n\n";
        std::cout << "�B���R�}���h������炵��...?(�R�i�~�R�}���h���Ă������)\n";

        // Konami Code�̓��͂�҂�
        const std::vector<int> konamiCode = { 72, 72, 80, 80, 75, 77, 75, 77, 'b', 'a' };
        std::vector<int> inputSequence;
        std::string input_str;

        while (true) {
            // _getch()��conio.h�Œ�`����Ă���L�[�{�[�h����1�������͂���֐�
            int ch = _getch();
            // Enter�L�[�������ꂽ�烋�[�v�𔲂���
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

            // ���̓V�[�P���X�̊m�F�@�R�i�~�R�}���h�ƈ�v���邩
            if (inputSequence.size() == konamiCode.size()) {
                if (inputSequence == konamiCode) {
                    GameStateManager::getInstance().setSecretStageMode(true);
                    std::cout << "\n\n*** �B���X�e�[�W���o�������I ***\n";
                    Sleep(1500);
                    state = GameState::StageIntro;
                    break;
                }
                inputSequence.erase(inputSequence.begin());
            }
        }
    }
};

// === ���[�e�B���e�B�֐� ===
void typeWriterPrint(const std::string& text, unsigned int delay_ms) {
    for (char c : text) {
        std::cout << c << std::flush;
        Sleep(delay_ms);
    }
    std::cout << std::endl;
}

// === �X�e�[�W�J�n�V�[�� ===
class StageIntroScene : public Scene {
public:
    void update(GameState& state) override {
        system("cls");
        const auto& config = GameStateManager::getInstance().getCurrentStage();
        std::cout << "------------------------------------------\n";
        std::cout << "=== " << config.title << " ===\n";
        std::cout << "------------------------------------------\n\n";

        // --- �Z���t�p�[�g ---
        for (const auto& line : config.intro_dialogues) {
            typeWriterPrint(line, 20); // 50�~���b�ҋ@
            Sleep(500); // ���̃Z���t�܂ł̊�
        }

        std::cout << "�ړI: " << config.description << "\n\n";
        std::cout << "[Enter]�Œ���J�n...\n";

        // cin�̃o�b�t�@���N���A
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::cin.get();
        state = GameState::Playing;
    }
};

// === �v���C�V�[�� ===
class PlayScene : public Scene {
    const StageConfig& config;
    std::string answer;
    PoolAllocator<Attempt, 20> pool;
    std::vector<Attempt*> attempts;
    bool isTutorial;

    std::string generateAnswer() {
        std::string digits = "0123456789";
        // 0~config.num_range�̐������g�p���āAconfig.digits���̐����𐶐�
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
            std::cout << "\n*** SKILL: LEAK �����I ***\n";
            std::cout << "�I�y���[�^�[:�u�����Ɋ܂܂�鐔����... �u" << answer[rand() % config.digits] << "�v���B�v\n";
            gm.useLeak();
        }
        else if (skill == "sum" && gm.canUseSum()) {
            int sum = 0;
            for (char c : answer) { sum += c - '0'; }
            std::cout << "\n*** SKILL: SUM �����I ***\n";
            std::cout << "�I�y���[�^�[:�u�����̐����̍��v�l��... �u" << sum << "�v���B�v\n";
            gm.useSum();
        }
        else if (skill == "scan" && gm.canUseScan()) {
            std::cout << "\n*** SKILL: SCAN �����I ***\n";
            int pos;
            std::cout << "�X�L�������錅����͂��Ă������� (1-" << config.digits << "): ";
            std::cin >> pos;
            if (std::cin.fail() || pos < 1 || pos > config.digits) {
                std::cout << "�I�y���[�^�[:�u���̌��͖������B���������ă^�C�s���O���悤�B�v(Enter�����ē��͂ɖ߂�)\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            else {
                std::cout << "�I�y���[�^�[:�u�w�肳�ꂽ" << pos << "���ڂ̐�����... �u" << answer[pos - 1] << "�v���B�v\n";
                gm.useScan();
            }
        }
        else {
            std::cout << "\n�I�y���[�^�[:�u���̃X�L���͑��݂��Ȃ����A���Ɏg�p�ς݂��v�B\n";
        }
        std::cout << "[Enter]�ő��s...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }


public:
    PlayScene(const StageConfig& cfg) : config(cfg) {
        answer = generateAnswer();
        // std::cout << "DEBUG: " << answer << "\n"; // �f�o�b�O�p
        // �X�e�[�W�C���f�b�N�X��0�A���B���X�e�[�W�łȂ��Ȃ�`���[�g���A���Ɣ���
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
                "�i�ߊ�:�u�`���[�g���A�����J�n����B�v",
                "�i�ߊ�:�u�N�̖ړI�́A�B���ꂽ�����������̕��т𓖂Ă邱�Ƃ��B�v",
                "�i�ߊ�:�u���͂��������ɑ΂���q���g��2��ށA�wHit�x�ƁwBlow�x�ŗ^������B�v",
                "",
                "�yHit�z : �������ʒu�������Ă��鐔�B",
                "�yBlow�z: �����͍����Ă��邪�A�ʒu���Ⴄ���B",
                "",
                "�i�ߊ�:�u��������ɁA�����𐄗�����񂾁B�v",
                "�i�ߊ�:�u����ɁA�n�b�L���O��⏕���鋭�͂ȁw�X�L���x��3��ށA�e1�񂾂��g�p�ł���B�v",
                "",
                "�yleak�z: �����̐����̂����A1�������_���ɋ����Ă����B",
                "�ysum�z : �����̐��������ׂđ��������v�l�������Ă����B",
                "�yscan�z: �w�肵���w���x�̐��������ł��邩�𐳊m�ɋ����Ă����B",
                "",
                "�i�ߊ�:�u�X�L���̓R�}���h�Ƃ��ē��͂��邱�ƂŔ�������B�g���ǂ��낪�d�v�����B�v",
                "�i�ߊ�:�u�����͈ȏゾ�B�K�^���F��B�v"
            };
            for (const auto& line : tutorial_lines) {
                typeWriterPrint(line, 30);
                if (!line.empty()) Sleep(100);
            }

            std::cout << "\n[Enter]�Ń~�b�V�����J�n...";
            std::cin.get();
        }
        std::string input;
        while (true) {
            system("cls");
            std::cout << "--- " << config.title << " ---\n";
            std::cout << "�c�莎�s��: " << (config.max_attempts - attempts.size()) << "\n\n";
            // ����\��
            for (const auto& a : attempts) {
                std::cout << a->guess << " �� Hit: " << a->hit << ", Blow: " << a->blow << "\n";
            };


            std::cout << "\n--- ���� -------------------------------\n";
            std::cout << "�ړI: " << config.description << "\n\n";

            std::cout << config.digits << "���A�͈͓��̐�������͂��Ă������� (�d���Ȃ�)�B\n\n";
            std::cout << "�X�L�� (�Q�[�����e1��̂�):\n";
            std::cout << "[leak]: �����̐����̂����A1�������_���ɋ����Ă����B\n";
            std::cout << "[sum] : �����̐��������ׂđ��������v�l�������Ă����B\n";
            std::cout << "[scan]: �w�肵�����̐����𐳊m�ɋ����Ă����B\n\n";
            std::cout << "[q]�ŃQ�[�����I�����܂��B\n";
            std::cout << "----------------------------------------\n";
            std::cout << attempts.size() + 1 << " ��ڂ̓���: ";
            std::cin >> input;

            if (input == "q") {
                state = GameState::Exit;
                return;
            }

            // �X�L������
            if (input == "leak" || input == "sum" || input == "scan") {
                executeSkill(input);
                continue;
            }

            // ���̓`�F�b�N
            if (static_cast<int>(input.length()) != config.digits || !std::all_of(input.begin(), input.end(), ::isdigit)) {
                std::cout << "�I�y���[�^�[:�u�����ȓ��͂��B�v (" << config.digits << "���̐����ł͂���܂���)(Enter�����ē��͂ɖ߂�)\n";
                std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                std::cin.get();
                continue;
            }


            // �����͈̔̓`�F�b�N
            bool validRange = true;
            for (char c : input) {
                if (static_cast<int>(c - '0') > config.num_range) {
                    validRange = false;
                    break;
                }
            }
            if (!validRange) {
                std::cout << "�I�y���[�^�[:�u�����͈̔͂��������B�v(0-" << config.num_range << "�̐������g�p���Ă�������)(Enter�����ē��͂ɖ߂�)\n";
                std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
                std::cin.get();
                continue;
            }

            // �d���`�F�b�N
            std::set<char> unique(input.begin(), input.end());
            if (static_cast<int>(unique.size()) != config.digits) {
                std::cout << "�I�y���[�^�[:�u�������d�����Ă��邼�I�v(Enter�����ē��͂ɖ߂�)\n";
                std::cin.get();
                std::cin.get();
                continue;
            }

            // ���菈��
            Attempt result = evaluate(input);

            // PoolAllocator���烁�����m��
            Attempt* a = pool.Alloc();
            if (a) {
                *a = result;
                attempts.push_back(a);
            }
            else {
                std::cout << "�������s���I\n";
                state = GameState::Result;
                return;
            }

            // �N���A����
            if (result.hit == config.digits) {
                std::cout << "\n*** LOCK OPEN! ***\n";
                std::cout << "\n�i�ߊ�:�u�������I�ł��������I������ " << answer << " ���B�v\n";
                std::cout << "[Enter]�Ŏ���...\n";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
                state = GameState::StageClear;
                return;
            }


            // �Q�[���I�[�o�[����
            if (static_cast<int>(attempts.size()) >= config.max_attempts) {
                system("cls");
                std::cout << "\n--- MISSION FAILED ---\n";
                std::cout << "�i�ߊ�:�u���Ԃ����肷����...�B������ " << answer << " ���������B�v\n";
                std::cout << "�i�ߊ�:�u�������͎��Ɋ������B�c�c�K���D���Ԃ����B�v\n\n";
                std::cout << "[Enter]�Ŏ���...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
                state = GameState::GameOver;
                return;
            }
        }
    }
};

// === ���ʃV�[�� ===
class ResultScene : public Scene {
public:
    void update(GameState& state) override {
        system("cls");
        auto& gm = GameStateManager::getInstance();
        std::cout << "========================\n";
        std::cout << "       ���U���g\n";
        std::cout << "========================\n\n";

        if (gm.getSecretStageMode()) {
            if (gm.isAllStagesCleared()) {
                std::cout << "�B���X�e�[�W�������N���A�I\n";
            }
            else {
                std::cout << "�B���X�e�[�W�N���A�Ȃ炸...\n";
            }
        }
        else {
            int cleared_stages = gm.getCurrentStageIndex();
            if (gm.isAllStagesCleared()) {
                cleared_stages++;
            }
            std::cout << "�N���A�X�e�[�W: " << cleared_stages << " / " << gm.getStageCount() << "\n\n";
            if (gm.isAllStagesCleared()) {
                std::cout << "�i�ߊ�:�u�c�c�C���������BNOC���X�g�A���S�D�ҁI�v\n";
                std::cout << "�i�ߊ�:�u�悭������A�G�[�W�F���g�B�N�͐��E���~�����񂾁B�v\n";
                std::cout << "�I�y���[�^�[:�u������!�{���ɂ�������I�N�͂����A�`���̃n�b�J�[���I\n";
                std::cout << "�i�ߊ�:�u�x�߁A�p�Y�B�����\�\���̐��́A�����ɂ���Ă���B�v\n\n";

            }
        }

        if (!gm.canUseLeak()) std::cout << "[SKILL: LEAK] �g�p�ς�\n";
        if (!gm.canUseSum()) std::cout << "[SKILL: SUM] �g�p�ς�\n";
        if (!gm.canUseScan()) std::cout << "[SKILL: SCAN] �g�p�ς�\n";

        std::cout << "\n[r]�ł�����x���� / [q]�ŏI��\n";
        char choice;
        std::cin >> choice;
        if (choice == 'r') {
            GameStateManager::getInstance().resetGame();
            state = GameState::Title; // �^�C�g���ɖ߂�
        }
        else state = GameState::Exit;
    }
};

// === ���C���֐� ===
int main() {
    // �G�X�P�[�v�V�[�P���X�̗L�����iWindows��p�j
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdOut, &mode);
    SetConsoleMode(hStdOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    printf("\x1b[?25l"); // �J�[�\����\��

    GameState state = GameState::Title;
    Scene* currentScene = nullptr;

    while (state != GameState::Exit) {
        // ���݂̃V�[�������
        delete currentScene;
        currentScene = nullptr;

        // �Q�[����Ԃɉ����ăV�[����؂�ւ�
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
        { // case���ŕϐ��錾���邽�߃X�R�[�v��ǉ�
            auto& gm = GameStateManager::getInstance();

            if (gm.getSecretStageMode()) {
                gm.ToNextStage();
                state = GameState::Result;
                continue;
            }

            // ���݂̃X�e�[�W���ŏI�X�e�[�W���ǂ������C���f�b�N�X�Œ��ڔ��肷��
            bool isLastStage = (gm.getCurrentStageIndex() >= gm.getStageCount() - 1);

            gm.ToNextStage(); // �X�e�[�W�����X�V

            if (isLastStage) {
                state = GameState::Result;      // �ŏI�X�e�[�W�N���A��̓��U���g��
            }
            else {
                state = GameState::StageIntro;  // ����ȊO�͎��̃X�e�[�W��
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

    // �J�[�\����\��
    printf("\x1b[?25h");
    return EXIT_SUCCESS;
}
