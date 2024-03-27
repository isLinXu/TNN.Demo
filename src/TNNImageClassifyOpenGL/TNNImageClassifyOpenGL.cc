// Tencent is pleased to support the open source community by making TNN available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the 
// specific language governing permissions and limitations under the License.

#include <fstream>
#include <string>
#include <vector>

#include "image_classifier.h"
#include "macro.h"
#include "utils/utils.h"
#include "opengl_direct_mem_adapter.h"

#include "../flags.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../third_party/stb/stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb/stb_image_write.h"

using namespace TNN_NS;

static const char label_path_message[] = "(optional) label file path. Default is: ../../../assets/synset.txt";
DEFINE_string(l, "../../../assets/synset.txt", label_path_message);

int main(int argc, char** argv) {
    if (!ParseAndCheckCommandLine(argc, argv)) {
        ShowUsage(argv[0]);
        printf("\t-l, <label>    \t%s\n", label_path_message);
        return -1;
    }

#if defined(SHARING_MEM_WITH_OPENGL) && (CL_HPP_TARGET_OPENCL_VERSION >= 120)
    std::shared_ptr<OpenGLDirectMemAdapter> adapter = std::make_shared<OpenGLDirectMemAdapter>();
#endif

    // 创建tnn实例
    auto proto_content = fdLoadFile(FLAGS_p.c_str());
    auto model_content = fdLoadFile(FLAGS_m.c_str());

    auto option = std::make_shared<TNNSDKOption>();
    {
        option->proto_content = proto_content;
        option->model_content = model_content;
        option->library_path = "";
        option->compute_units = TNN_NS::TNNComputeUnitsCPU;
        // if enable openvino, set option compute_units to openvino
        // if enable openvino/tensorrt, set option compute_units to openvino/tensorrt
        #if defined(_CUDA_) || defined(_OPENCL_)
            option->compute_units = TNN_NS::TNNComputeUnitsGPU;
        #endif
    }

    auto predictor = std::make_shared<ImageClassifier>();

    char* temp_p;
    char line[256];
    FILE *fp_label;
    if((fp_label = fopen(FLAGS_l.c_str(), "r")) == NULL) {
        fprintf(stderr, "ImageClassifier open lable file %s failed.\n", FLAGS_l.c_str());
        return -1;
    }
    static unsigned char labels[1000][256];
    for(int i = 0; i < 1000; i++){
        temp_p = fgets(line, 256 ,fp_label);
        memcpy(labels[i], line, 256);
    }
    fclose(fp_label);

    char img_buff[256];
    char *input_imgfn = img_buff;
    strncpy(input_imgfn, FLAGS_i.c_str(), 256);
    printf("Classify is about to start, and the picture is %s\n",input_imgfn);

    int image_width, image_height, image_channel;
    unsigned char *data = stbi_load(input_imgfn, &image_width, &image_height, &image_channel, 3);
    if (!data) {
        fprintf(stderr, "ImageClassifier open file %s failed.\n", input_imgfn);
    }

    std::vector<int> nchw = {1, image_channel, image_height, image_width};

    // Init
    std::shared_ptr<TNNSDKOutput> sdk_output = predictor->CreateSDKOutput();
    CHECK_TNN_STATUS(predictor->Init(option));
    auto image_mat = std::make_shared<TNN_NS::Mat>(TNN_NS::DEVICE_NAIVE, TNN_NS::N8UC3, nchw, data);

    auto net_input_mat = std::make_shared<TNN_NS::Mat>(TNN_NS::DEVICE_NAIVE, TNN_NS::N8UC3, predictor->GetInputShape());
    ResizeParam param;
    param.type = INTERP_TYPE_LINEAR;
    
    auto dst_dims = net_input_mat->GetDims();
    auto src_dims = image_mat->GetDims();
    param.scale_w = dst_dims[3] / static_cast<float>(src_dims[3]);
    param.scale_h = dst_dims[2] / static_cast<float>(src_dims[2]);
    MatUtils::Resize(*(image_mat.get()), *(net_input_mat.get()), param, nullptr);

    std::string device = "opencl";
#if defined(SHARING_MEM_WITH_OPENGL) && (CL_HPP_TARGET_OPENCL_VERSION >= 120)
    std::shared_ptr<Mat> target_input_mat;

    // opengl shared mem with opencl
    void *command_queue = nullptr;
    predictor->GetCommandQueue(&command_queue);
    adapter->Transform(net_input_mat, target_input_mat, (cl::CommandQueue *)command_queue);

    std::shared_ptr<TNNSDKInput> input = std::make_shared<TNNSDKInput>(target_input_mat);

    device = "opencl-opengl";
#else
    std::shared_ptr<TNNSDKInput> input = std::make_shared<TNNSDKInput>(net_input_mat);
#endif
    // Predict
    CHECK_TNN_STATUS(predictor->Predict(input, sdk_output));

    int class_id = -1;
    if (sdk_output && dynamic_cast<ImageClassifierOutput *>(sdk_output.get())) {
        auto classfy_output = dynamic_cast<ImageClassifierOutput *>(sdk_output.get());
        class_id = classfy_output->class_id;
    }
    fprintf(stdout, "Classify done. Device: %s, Result: %s, Output argmax: %d\n", device.c_str(), labels[class_id], class_id+1);
    fprintf(stdout, "%s\n", predictor->GetBenchResult().Description().c_str());
    free(data);
    return 0;
}
