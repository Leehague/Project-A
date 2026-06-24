#pragma once
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <memory>
#include <iostream>
#include <windows.h>
#include <filesystem>
#include <iostream>

class RLModelManager
{
public:
    static RLModelManager& GetInstance()
    {
        static RLModelManager instance;
        return instance;
    }


    void Init(const wchar_t* modelPath)
    {
        try
        {
            _env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "RL_Monster_Model");

            Ort::SessionOptions sessionOptions;
            sessionOptions.SetIntraOpNumThreads(1);
            sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

            // 1. 실행 파일(.exe)의 절대 경로 구하기
            wchar_t exePathBuffer[MAX_PATH];
            GetModuleFileNameW(NULL, exePathBuffer, MAX_PATH);
            std::filesystem::path exeDir = std::filesystem::path(exePathBuffer).parent_path();

            // 2. 기본 후보 경로 (실행 파일과 같은 폴더 내)
            std::filesystem::path targetModelPath = exeDir / modelPath;

            // 3. 만약 파일이 없고 상위 폴더(프로젝트 폴더)에 존재한다면 fallback 처리
            if (!std::filesystem::exists(targetModelPath))
            {
                // x64/Debug 폴더의 상위의 상위 폴더(Project_TOY_server) 기준 검색
                std::filesystem::path fallbackPath = exeDir / L"../../" / modelPath;
                if (std::filesystem::exists(fallbackPath))
                {
                    targetModelPath = fallbackPath;
                }
                else
                {
                    // 옆의 Project_TOY_RL 프로젝트 폴더 기준 검색 (솔루션 루트 상위)
                    std::filesystem::path rlProjectPath = exeDir / L"../../../Project_TOY_RL/" / modelPath;
                    if (std::filesystem::exists(rlProjectPath))
                    {
                        targetModelPath = rlProjectPath;
                    }
                }
            }

            // 4. 결정된 경로로 로드 실행
            _session = std::make_unique<Ort::Session>(_env, targetModelPath.c_str(), sessionOptions);
            
            // 모델의 예상 입력 차원(State Space Dimension) 추출
            size_t numInputNodes = _session->GetInputCount();
            if (numInputNodes > 0)
            {
                Ort::TypeInfo typeInfo = _session->GetInputTypeInfo(0);
                auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
                std::vector<int64_t> inputShape = tensorInfo.GetShape();
                if (inputShape.size() > 1)
                {
                    _expectedInputDim = static_cast<size_t>(inputShape[1]);
                }
                else if (!inputShape.empty())
                {
                    _expectedInputDim = static_cast<size_t>(inputShape[0]);
                }
            }

            std::wcout << L"[SUCCESS] ONNX model loaded from: " << targetModelPath.wstring() 
                       << L" (Expected Input Dimension: " << _expectedInputDim << L")" << std::endl;
        }
        catch (const Ort::Exception& e)
        {
            std::cerr << "[ERROR] Failed to initialize ONNX Runtime Session: " << e.what() << std::endl;
            std::wcerr << L"[ERROR] Requested path was: " << modelPath << std::endl;
            _session = nullptr;
        }
    }


    int Predict(const std::vector<float>& context)
    {
        if (!_session) return 0; // 모델이 없는 경우 기본 액션(0) 예외처리

        // 모델이 요구하는 입력 차원과 제공된 컨텍스트 크기 정합성 검증
        if (_expectedInputDim > 0 && context.size() != _expectedInputDim)
        {
            std::cerr << "[ERROR] Input dimension mismatch! Model expects " << _expectedInputDim
                      << ", but context size is " << context.size() << std::endl;
            return 0; // 예외 발생 시 기본 액션(0) 반환
        }

        // 1. 입력 모양 및 텐서 생성
        std::vector<int64_t> inputShape = { 1, static_cast<int64_t>(context.size()) };
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(
            OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

        std::vector<float> inputData = context;
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, inputData.data(), inputData.size(), inputShape.data(), inputShape.size());

        const char* inputNames[] = { "input" };
        const char* outputNames[] = { "output" };

        // 2. 모델 추론 실행
        auto outputTensors = _session->Run(
            Ort::RunOptions{ nullptr },
            inputNames,
            &inputTensor,
            1,
            outputNames,
            1
        );

        // 3. 결과 파싱 (torch.argmax 출력이므로 int64_t 형식으로 데이터 추출)
        int64_t* outputData = outputTensors[0].GetTensorMutableData<int64_t>();
        return static_cast<int>(outputData[0]);
    }

private:
    RLModelManager() = default;
    Ort::Env _env;
    std::unique_ptr<Ort::Session> _session;
    size_t _expectedInputDim = 0; // 모델이 요구하는 입력 차원 (State Space Dimension)
};
