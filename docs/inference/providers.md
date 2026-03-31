# Providers

Vix does not ship an inference engine.

Instead, you plug in a provider.

A provider is simply the code that runs inference.

Examples of providers:

-   ONNX Runtime
-   TensorRT
-   OpenVINO
-   A custom C++ model
-   A remote inference service

Vix only handles HTTP.

------------------------------------------------------------------------

## Minimal provider concept

A provider takes input and returns output.

Example: a fake provider inside `main`.

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/predict", [](Request& req, Response& res) {
    const std::string input = req.query_value("input", "");

    // Provider logic (minimal example)
    std::string prediction = "pred_" + input;

    res.json({
      "input", input,
      "prediction", prediction
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Provider with JSON input

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/infer", [](Request& req, Response& res) {
    const auto& j = req.json();

    // Minimal provider style
    std::string model = "demo-model";
    std::string result = "ok";

    res.json({
      "model", model,
      "input", j,
      "result", result
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Provider selection strategy

Pick based on your constraints:

-   CPU-only inference
-   GPU acceleration
-   Platform targets
-   Model format compatibility
-   Deployment simplicity

Vix stays the same. Only the provider changes.

------------------------------------------------------------------------

## Production notes

When you use a real provider:

-   Load the model once at startup
-   Reuse sessions and buffers
-   Avoid reloading per request
-   Validate input strictly
-   Avoid leaking model internals in errors

------------------------------------------------------------------------

## Common integration style

In practice you will do:

-   Initialize provider at startup
-   Handle requests
-   Call provider
-   Return JSON output

Even in production, keep the route handler minimal.

------------------------------------------------------------------------

## Philosophy

Providers are optional.

Vix is a runtime. Inference is a module you bring.

