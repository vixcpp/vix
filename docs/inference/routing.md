# Routing for inference

Inference endpoints are normal Vix routes.

The difference is what you do inside the handler:

-   Validate input
-   Run model inference (provider call)
-   Format output

Keep the route handler simple.

------------------------------------------------------------------------

## Query-based inference

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/predict", [](Request& req, Response& res) {
    const std::string input = req.query_value("input", "");

    if (input.empty())
    {
      res.status(400).json({
        "error", "Missing query param",
        "param", "input"
      });
      return;
    }

    // Simulated inference
    const std::string output = "pred_" + input;

    res.json({
      "input", input,
      "prediction", output
    });
  });

  app.run(8080);
}
```

Test:

``` bash
curl "http://127.0.0.1:8080/predict?input=hello"
```

------------------------------------------------------------------------

## JSON-based inference

``` cpp
#include <vix.hpp>

using namespace vix;

int main()
{
  App app;

  app.get("/infer", [](Request& req, Response& res) {
    const auto& j = req.json();

    if (!j.is_object())
    {
      res.status(400).json({
        "error", "Expected a JSON object"
      });
      return;
    }

    // Simulated inference output
    res.json({
      "input", j,
      "result", "ok"
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Batch inference pattern

Batch means: one request contains multiple inputs.

``` cpp
#include <vix.hpp>

using namespace vix;
namespace J = vix::json;

int main()
{
  App app;

  app.get("/batch", [](Request& req, Response& res) {
    const auto& j = req.json();

    if (!j.is_object() || !j.contains("inputs") || !j["inputs"].is_array())
    {
      res.status(400).json({
        "error", "Expected JSON with inputs: []"
      });
      return;
    }

    // Simulated inference over inputs
    res.json({
      "inputs", j["inputs"],
      "predictions", J::array({"pred_1", "pred_2"})
    });
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Input size guard

Inference endpoints should protect the runtime.

``` cpp
#include <vix.hpp>
#include <string>

using namespace vix;

int main()
{
  App app;

  app.get("/safe", [](Request& req, Response& res) {
    const std::string body = req.body();

    if (body.size() > 1024 * 1024)
    {
      res.status(413).json({
        "error", "Payload too large"
      });
      return;
    }

    res.json({"ok", true, "bytes", (long long)body.size()});
  });

  app.run(8080);
}
```

------------------------------------------------------------------------

## Rule of thumb

For inference routing:

-   Validate early
-   Reject bad inputs fast
-   Keep responses consistent
-   Avoid heavy allocations in hot paths
-   Load models once, not per request

