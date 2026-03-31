# Reliability

This page explains reliability guarantees and failure behavior in Vix
Inference.

Inference in the real world fails for boring reasons:

-   provider rate limits
-   network timeouts
-   model cold starts
-   overloaded GPUs
-   partial responses during streaming
-   invalid JSON from clients
-   slow clients or disconnects

The goal is not "never fail". The goal is predictable behavior under
failure.

------------------------------------------------------------------------

## Reliability goals

Vix Inference is designed around these principles:

-   Fast failure for invalid requests
-   Retries for transient provider errors
-   Clear error codes and messages
-   Safe streaming termination
-   Observability (logs + request id)
-   No silent partial success

------------------------------------------------------------------------

## Request lifecycle

A typical request goes through:

1)  validate input (schema and limits)
2)  select provider and model
3)  execute inference with timeouts
4)  stream or buffer output
5)  finalize and emit metrics

If any step fails, Vix returns a structured error.

------------------------------------------------------------------------

## Timeouts

Always set timeouts.

Recommended timeouts:

-   connect timeout: 2 to 5 seconds
-   first token timeout: 5 to 15 seconds
-   total request timeout: 30 to 120 seconds (depends on workload)
-   per chunk flush interval: small and constant

Timeouts must be enforced both:

-   at transport level (HTTP / WS)
-   at provider level (SDK / HTTP client)

------------------------------------------------------------------------

## Retries

Retries should be limited and only for transient failures.

Good retry candidates:

-   429 Too Many Requests
-   503 Service Unavailable
-   network timeouts
-   connection reset

Bad retry candidates:

-   400 Bad Request
-   401 Unauthorized
-   403 Forbidden
-   404 Not Found
-   422 Validation errors

Recommended strategy:

-   max attempts: 2 or 3
-   exponential backoff
-   jitter
-   retry budget per request

------------------------------------------------------------------------

## Idempotency

Inference is usually safe to retry because:

-   it does not mutate server state
-   it returns generated text

But streaming can duplicate partial tokens if you retry mid-stream.

If you support retries on the client:

-   retry only before the first token
-   or use an idempotency key and server-side replay

------------------------------------------------------------------------

## Streaming reliability

Streaming has extra failure modes:

-   client disconnects mid-stream
-   provider disconnects mid-stream
-   chunk parsing errors
-   backpressure and slow readers

Rules:

-   detect disconnect and cancel inference
-   always send an explicit end event when possible
-   do not keep streaming forever
-   cap total streamed bytes

Recommended events:

-   `inference.start`
-   `inference.token`
-   `inference.error`
-   `inference.end`

If the client disconnects, Vix must stop work quickly.

------------------------------------------------------------------------

## Circuit breaker

If a provider is failing repeatedly, stop sending traffic to it.

A minimal circuit breaker uses:

-   failure counter in a short window
-   open state for a cooldown period
-   half-open probe requests

This protects your system from cascading failure.

------------------------------------------------------------------------

## Fallback providers

If you configure multiple providers, you can fail over.

Example behavior:

-   prefer local provider
-   if it fails with timeout or 503, try remote provider
-   if all fail, return an error with attempts info

Failover must be visible in logs and metrics.

------------------------------------------------------------------------

## Error format

A reliable API returns predictable errors.

Recommended error envelope:

``` json
{
  "ok": false,
  "error": {
    "code": "provider_timeout",
    "message": "Provider did not respond in time",
    "retryable": true
  },
  "request_id": "...."
}
```

For streaming, send the same envelope inside `inference.error`.

------------------------------------------------------------------------

## Limits

Reliability includes protecting resources.

Common limits:

-   max input tokens
-   max output tokens
-   max request body bytes
-   max concurrent streams
-   max requests per minute per key
-   max streaming duration

Reject early with a clear 4xx error.

------------------------------------------------------------------------

## Observability

To debug production failures you need:

-   request id in every response
-   structured logs (provider, model, latency)
-   metrics (p50/p95/p99, error rates)
-   retry counts and final reason

Minimal log fields:

-   request_id
-   provider
-   model
-   status
-   latency_ms
-   retry_count
-   streaming (true/false)

------------------------------------------------------------------------

## Practical checklist

Before shipping:

-   timeouts configured
-   input validation enforced
-   retry policy defined
-   streaming end event implemented
-   circuit breaker enabled
-   rate limit enabled
-   logs and metrics enabled

Reliability is not a feature. It is the default behavior.

