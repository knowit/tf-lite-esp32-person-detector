#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "face_model.h"
#include "image_scaler.h"

#include <Arduino.h>
#include <esp32cam.h>

tflite::MicroErrorReporter micro_error_reporter;
tflite::AllOpsResolver resolver;
const tflite::Model* model;
tflite::MicroInterpreter* interpreter;

constexpr int kTensorArenaSize = 64*1024;
static uint8_t tensor_arena[kTensorArenaSize];

constexpr int scaled_size = 32*32;
static uint8_t scaled_buffer[scaled_size];

const int frame_width = 160;
const int frame_height = 120;
static auto res = esp32cam::Resolution::find(frame_width, frame_height);

void quantize(uint8_t* data, int8_t* quantized_data, uint32_t length) {
  float input_scale = 0.00392118f;
  int input_zero_point = -128;

  for (auto i = 0; i < length; i++) {
    auto d = data[i];
    auto scaled_d = static_cast<float>(d)/255.0f;
    quantized_data[i] = static_cast<int8_t>(static_cast<int>(scaled_d / input_scale) + input_zero_point);
  }
}

float dequantize(int8_t quantized_data) {
  float output_scale = 0.00390625f;
  int output_zero_point = -128;

  return static_cast<float>(quantized_data - output_zero_point) * output_scale;
}

void setup() {
  Serial.begin(9600);

  // Load TF lite model
  model = tflite::GetModel(face_model_optimized_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Wrong model schema version");
    return;
  }
  Serial.print("TF Lite schema version: ");
  Serial.print(model->version());
  Serial.println();

  // Initialize TF lite interpreter
  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);
  interpreter = &static_interpreter;
  Serial.println("Interpreter ready");

  // Allocate tensors
  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("Tensor alloc failed");
    return;
  }
  Serial.println("Tensors ready");

  
  esp32cam::Config cfg;
  cfg.setPins(esp32cam::pins::AiThinker);
  cfg.setResolution(res);
  cfg.setBufferCount(1);
  cfg.setGrayscale();

  auto ok = esp32cam::Camera.begin(cfg);
  if (!ok) {
    Serial.println("Camear setup fail");
    return;
  }
  Serial.println("Camera ready");
  
  Serial.println("Setup complete");
}

void loop() {
  // Get camera frame and downscale to 32x32
  auto frame = esp32cam::capture();
  auto buffer = frame->data();
  img::downscale(buffer, frame_width, frame_height, scaled_buffer, 32, 32);

  // Scale and quantize input
  auto input = interpreter->input(0);
  quantize(scaled_buffer, input->data.int8, scaled_size);

  // Invoke TF lite interpreter
  auto invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("invoke failed");
  }

  // Dequantize output
  auto output = interpreter->output(0);
  auto pred = dequantize(output->data.int8[0]);


  // Greet human if over 25% confidence  
  static int human_seen = 0;

  if (pred >= 0.25f) {
    human_seen = 1;
    Serial.print("Hello human (");
    Serial.print(pred*100);
    Serial.println("% conf)");
  } else {
    human_seen = human_seen == 1 ? 2 : 0;
  }

  if (human_seen > 1) {
    Serial.println("Bye human");
    human_seen = 0;
  }
}