# Convergio V7: Architecture Deep Dive - Core C Deployment & Scalability

## The Core Problem

**Question:** Il core è scritto in C. Come lo facciamo girare in un ambiente SaaS? Come gestiamo deployment, distribuzione, scalabilità?

**Reality Check:**
- Core C è un processo singolo, non un servizio web
- Non ha API HTTP nativa
- Non gestisce multi-tenancy
- Non scala orizzontalmente di default
- Richiede compilazione per ogni architettura
- Gestione manuale di processi, memoria, crash

---

## Architecture Options Analysis

### Option 1: Core C as Library + API Gateway (Recommended)

**Architecture:**

```
┌─────────────────────────────────────────────────────────┐
│                    Load Balancer                         │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        ▼                         ▼
┌──────────────┐         ┌──────────────┐
│  API Gateway │         │  Web Server  │
│  (Rust/Go)   │         │ (Next.js)    │
└──────┬───────┘         └──────────────┘
       │
       ├──► Auth Service
       ├──► License Service
       │
       └──► Core Service Pool (Rust/Go)
            │
            ├──► convergio_core.so (C library)
            │    ├── orchestrator
            │    ├── llm_router
            │    ├── tool_engine
            │    └── memory
            │
            └──► Process Manager
                 ├── Worker Pool (N processes)
                 ├── Request Queue
                 └── State Management
```

**Implementation:**

1. **Refactor Core C as Library**
   - Convertire `main()` in `convergio_init()` / `convergio_run()`
   - Esporre API C pura (no CLI dependencies)
   - Rimuovere dipendenze da stdin/stdout

```c
// include/convergio/core_api.h

typedef struct {
    char* user_id;
    char* session_id;
    void* config;
    void* memory_store;
} ConvergioContext;

typedef struct {
    char* agent_id;
    char* prompt;
    char* conversation_id;
    void* metadata;
} ConvergioRequest;

typedef struct {
    char* response;
    bool is_streaming;
    void* stream_callback;
    int error_code;
    char* error_message;
} ConvergioResponse;

// Core API
ConvergioContext* convergio_init(const char* config_path);
ConvergioResponse* convergio_process_request(
    ConvergioContext* ctx,
    ConvergioRequest* req
);
void convergio_stream_response(
    ConvergioContext* ctx,
    ConvergioRequest* req,
    void (*callback)(const char* chunk, void* user_data),
    void* user_data
);
void convergio_cleanup(ConvergioContext* ctx);
```

2. **API Gateway in Rust/Go**
   - Wrapper che chiama la libreria C via FFI
   - Gestisce HTTP, WebSocket, autenticazione
   - Process pool per isolamento
   - Request queue per load balancing

```rust
// api_gateway/src/core_wrapper.rs

use libc::*;
use std::ffi::{CString, CStr};

#[repr(C)]
pub struct ConvergioContext {
    // Opaque pointer to C struct
    ptr: *mut c_void,
}

extern "C" {
    fn convergio_init(config_path: *const c_char) -> *mut ConvergioContext;
    fn convergio_process_request(
        ctx: *mut ConvergioContext,
        req: *const ConvergioRequest,
    ) -> *mut ConvergioResponse;
    fn convergio_cleanup(ctx: *mut ConvergioContext);
}

pub struct CoreService {
    workers: Vec<Worker>,
    request_queue: Arc<Mutex<VecDeque<Request>>>,
}

impl CoreService {
    pub fn new(worker_count: usize) -> Self {
        let workers = (0..worker_count)
            .map(|_| Worker::new())
            .collect();
        // ...
    }
    
    pub async fn process(&self, req: Request) -> Response {
        // Queue request, assign to worker
        // Worker calls C library via FFI
    }
}
```

**Pros:**
- ✅ Mantiene core C (performance, codebase esistente)
- ✅ Isolamento per processo (crash non affettano altri)
- ✅ Scalabilità orizzontale (più worker processes)
- ✅ Gestione memoria per processo
- ✅ Facile debugging (processi separati)

**Cons:**
- ❌ Overhead FFI (minimo, ma presente)
- ❌ Gestione processi più complessa
- ❌ State sharing richiede database esterno

**Deployment:**
- Core compilato come `.so` (Linux) / `.dylib` (macOS) / `.dll` (Windows)
- API Gateway compilato come binario Rust/Go
- Docker container con entrambi

---

### Option 2: Core C as Standalone Process + gRPC/HTTP

**Architecture:**

```
┌─────────────────────────────────────────────────────────┐
│                    API Gateway (Rust/Go)                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ HTTP Server  │  │ WebSocket    │  │  gRPC Server │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
└─────────┼─────────────────┼─────────────────┼──────────┘
          │                 │                 │
          └─────────────────┴─────────────────┘
                            │
                            │ IPC (gRPC/HTTP)
                            ▼
          ┌─────────────────────────────────┐
          │   Core Process Manager           │
          │  ┌─────────────────────────────┐ │
          │  │  Process Pool               │ │
          │  │  ┌─────┐ ┌─────┐ ┌─────┐   │ │
          │  │  │ C1  │ │ C2  │ │ C3  │   │ │
          │  │  └─────┘ └─────┘ └─────┘   │ │
          │  └─────────────────────────────┘ │
          │  ┌─────────────────────────────┐ │
          │  │  Request Router             │ │
          │  │  (Round-robin / Least-load) │ │
          │  └─────────────────────────────┘ │
          └─────────────────────────────────┘
```

**Implementation:**

1. **Core C with HTTP/gRPC Server**
   - Aggiungere server HTTP minimale (libmicrohttpd) o gRPC C
   - Endpoint: `/api/v1/process`, `/api/v1/stream`
   - JSON request/response

```c
// src/core/http_server.c

#include <microhttpd.h>

int handle_request(void* cls, struct MHD_Connection* connection,
                   const char* url, const char* method,
                   const char* version, const char* upload_data,
                   size_t* upload_data_size, void** con_cls) {
    
    if (strcmp(method, "POST") == 0 && strcmp(url, "/api/v1/process") == 0) {
        // Parse JSON request
        ConvergioRequest req = parse_request(upload_data);
        
        // Process with core
        ConvergioResponse* resp = convergio_process_request(&req);
        
        // Return JSON response
        return send_json_response(connection, resp);
    }
    return MHD_NO;
}

int main(int argc, char** argv) {
    struct MHD_Daemon* daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION,
        8080, NULL, NULL, &handle_request, NULL,
        MHD_OPTION_END
    );
    // ...
}
```

2. **Process Manager**
   - Spawn N processi core
   - Load balancer tra processi
   - Health checks, restart su crash

**Pros:**
- ✅ Isolamento completo (processi separati)
- ✅ Core rimane standalone
- ✅ Facile scaling (più processi)
- ✅ Crash isolation

**Cons:**
- ❌ Overhead IPC (HTTP/gRPC)
- ❌ Gestione processi complessa
- ❌ State sharing richiede DB esterno
- ❌ Aggiunge dipendenza (libmicrohttpd/gRPC)

---

### Option 3: Rewrite Core in Rust/Go (Long-term)

**Architecture:**

```
┌─────────────────────────────────────────────────────────┐
│              Convergio Core (Rust/Go)                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ Orchestrator │  │  LLM Router  │  │ Tool Engine  │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │   Memory     │  │   Plugin     │  │   HTTP/gRPC   │  │
│  │   System     │  │   System     │  │   Server      │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
```

**Pros:**
- ✅ Native HTTP/gRPC support
- ✅ Better concurrency (async/await)
- ✅ Memory safety
- ✅ Easier deployment (single binary)
- ✅ Better tooling (cargo, go modules)

**Cons:**
- ❌ Rewrite completo (6-12 mesi)
- ❌ Perdita codice esistente
- ❌ Risk di regressioni
- ❌ Team deve imparare nuovo linguaggio

**Recommendation:** Non ora, ma considerare per V8 se il progetto cresce.

---

## Recommended Architecture: Hybrid Approach

### Phase 1: Core C as Library + Rust API Gateway

**Why:**
- Mantiene codebase esistente
- Performance C mantenuta
- Isolamento via process pool
- Scalabilità orizzontale

**Implementation Details:**

#### 1. Core C Refactoring

```c
// src/core/core_api.c

// Remove CLI dependencies
// Remove stdin/stdout usage
// Add pure C API

typedef struct {
    char* user_id;
    char* session_id;
    ConvergioConfig* config;
    MemoryStore* memory;
    PluginRegistry* plugins;
} ConvergioContext;

ConvergioContext* convergio_init(const char* config_json) {
    ConvergioContext* ctx = malloc(sizeof(ConvergioContext));
    // Initialize core
    ctx->config = config_load_from_json(config_json);
    ctx->memory = memory_init(ctx->config->memory_path);
    ctx->plugins = plugin_registry_init();
    return ctx;
}

ConvergioResponse* convergio_process(
    ConvergioContext* ctx,
    const char* agent_id,
    const char* prompt,
    const char* conversation_id
) {
    // Core orchestration logic
    Agent* agent = plugin_get_agent(ctx->plugins, agent_id);
    LLMResponse* llm_resp = llm_router_call(agent, prompt);
    // ...
    return response;
}

void convergio_stream(
    ConvergioContext* ctx,
    const char* agent_id,
    const char* prompt,
    void (*chunk_callback)(const char* chunk, size_t len, void* user_data),
    void* user_data
) {
    // Streaming implementation
}
```

#### 2. Rust API Gateway

```rust
// api_gateway/src/main.rs

use std::sync::Arc;
use tokio::sync::Mutex;
use axum::{
    extract::State,
    routing::post,
    Router,
    Json,
};

struct AppState {
    core_pool: CoreProcessPool,
    db: Database,
    auth: AuthService,
}

#[tokio::main]
async fn main() {
    let state = AppState {
        core_pool: CoreProcessPool::new(10), // 10 worker processes
        db: Database::connect().await,
        auth: AuthService::new(),
    };
    
    let app = Router::new()
        .route("/api/v1/chat", post(handle_chat))
        .route("/api/v1/stream", post(handle_stream))
        .with_state(Arc::new(state));
    
    axum::Server::bind(&"0.0.0.0:3000".parse().unwrap())
        .serve(app.into_make_service())
        .await
        .unwrap();
}

async fn handle_chat(
    State(state): State<Arc<AppState>>,
    Json(req): Json<ChatRequest>,
) -> Json<ChatResponse> {
    // Authenticate
    let user = state.auth.verify(&req.token).await?;
    
    // Get worker from pool
    let worker = state.core_pool.get_worker().await;
    
    // Call C library via FFI
    let response = worker.process(&req).await?;
    
    // Log telemetry
    state.db.log_request(&user.id, &req, &response).await;
    
    Json(response)
}
```

#### 3. Process Pool Manager

```rust
// api_gateway/src/core_pool.rs

use std::sync::Arc;
use tokio::sync::Semaphore;

pub struct CoreProcessPool {
    workers: Vec<Arc<CoreWorker>>,
    semaphore: Arc<Semaphore>,
}

pub struct CoreWorker {
    context: *mut c_void, // ConvergioContext from C
    id: usize,
}

impl CoreProcessPool {
    pub fn new(count: usize) -> Self {
        let workers = (0..count)
            .map(|id| {
                let ctx = unsafe {
                    let config = CString::new("{}").unwrap();
                    convergio_init(config.as_ptr())
                };
                Arc::new(CoreWorker { context: ctx, id })
            })
            .collect();
        
        Self {
            workers,
            semaphore: Arc::new(Semaphore::new(count)),
        }
    }
    
    pub async fn get_worker(&self) -> Arc<CoreWorker> {
        let _permit = self.semaphore.acquire().await.unwrap();
        // Round-robin or least-load selection
        self.workers[rand::random::<usize>() % self.workers.len()].clone()
    }
}

impl CoreWorker {
    pub async fn process(&self, req: &ChatRequest) -> Result<ChatResponse> {
        // Call C function via FFI
        let c_req = convert_to_c_request(req);
        let c_resp = unsafe {
            convergio_process(self.context, &c_req)
        };
        convert_from_c_response(c_resp)
    }
}
```

---

## Deployment Strategy

### 1. Build System

```makefile
# Makefile

.PHONY: build-core build-api build-all

build-core:
	cd src/core && \
	gcc -shared -fPIC -o libconvergio_core.so \
		*.c orchestrator/*.c llm/*.c tools/*.c memory/*.c \
		-I../include

build-api:
	cd api_gateway && \
	cargo build --release

build-all: build-core build-api
	docker build -t convergio:latest .
```

### 2. Docker Container

```dockerfile
# Dockerfile

FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    libssl-dev \
    ca-certificates

# Copy core library
COPY build/libconvergio_core.so /usr/local/lib/
RUN ldconfig

# Copy API gateway binary
COPY api_gateway/target/release/convergio-api /usr/local/bin/

# Copy plugins
COPY plugins/ /opt/convergio/plugins/

# Run API gateway
CMD ["/usr/local/bin/convergio-api"]
```

### 3. Kubernetes Deployment

```yaml
# k8s/deployment.yaml

apiVersion: apps/v1
kind: Deployment
metadata:
  name: convergio-api
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: api-gateway
        image: convergio:latest
        ports:
        - containerPort: 3000
        env:
        - name: CORE_WORKER_COUNT
          value: "10"
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: convergio-secrets
              key: database-url
        resources:
          requests:
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "2Gi"
            cpu: "2000m"
---
apiVersion: v1
kind: Service
metadata:
  name: convergio-api
spec:
  selector:
    app: convergio-api
  ports:
  - port: 80
    targetPort: 3000
  type: LoadBalancer
```

---

## Scalability Considerations

### 1. Horizontal Scaling

- **Stateless API Gateway**: Può scalare orizzontalmente
- **State in Database**: Memory store in PostgreSQL/Redis
- **Session Affinity**: Non necessaria (stateless)

### 2. Vertical Scaling

- **Worker Pool Size**: Configurabile per pod
- **Memory per Worker**: ~100-500MB
- **CPU**: I/O bound (LLM calls), non CPU intensive

### 3. Database Scaling

- **PostgreSQL**: Per persistent state (conversations, memory)
- **Redis**: Per session cache, rate limiting
- **Read Replicas**: Per distribuire load

### 4. LLM Provider Scaling

- **Connection Pooling**: Riutilizzare connessioni HTTP
- **Rate Limiting**: Rispettare limiti provider
- **Caching**: Cache risposte simili (opzionale)

---

## What We're Missing: Critical Considerations

### 1. **State Management**

**Problem:** Core C mantiene stato in memoria. Come condividiamo tra processi?

**Solution:**
- Memory store esterno (PostgreSQL/Redis)
- Core C legge/scrive da DB invece di memoria locale
- Session state in Redis (TTL)

```c
// src/core/memory_external.c

typedef struct {
    DatabaseConnection* db;
    RedisConnection* cache;
} ExternalMemoryStore;

void memory_save(ExternalMemoryStore* store, const char* key, const char* value) {
    redis_set(store->cache, key, value, 3600); // 1 hour TTL
    db_insert(store->db, "memory", key, value);
}
```

### 2. **Error Handling & Crash Recovery**

**Problem:** C process crasha. Come recuperiamo?

**Solution:**
- Health checks periodici
- Auto-restart su crash (supervisor)
- Request timeout (30s default)
- Circuit breaker per LLM calls

```rust
// api_gateway/src/health.rs

impl CoreWorker {
    pub async fn health_check(&self) -> bool {
        // Ping core process
        let result = tokio::time::timeout(
            Duration::from_secs(1),
            self.ping()
        ).await;
        result.is_ok()
    }
}

// Supervisor
async fn supervise_workers(pool: &CoreProcessPool) {
    loop {
        for worker in &pool.workers {
            if !worker.health_check().await {
                // Restart worker
                pool.restart_worker(worker.id).await;
            }
        }
        tokio::time::sleep(Duration::from_secs(10)).await;
    }
}
```

### 3. **Resource Limits**

**Problem:** Un utente può consumare troppe risorse.

**Solution:**
- Memory limits per worker
- CPU time limits
- Request rate limiting
- Timeout per richieste lunghe

```rust
// Rate limiting
use governor::{Quota, RateLimiter};

let limiter = RateLimiter::keyed(Quota::per_minute(nonzero!(100)));

if limiter.check_key(&user_id).is_err() {
    return Err("Rate limit exceeded");
}
```

### 4. **Security**

**Problem:** Isolamento tra tenant, sandboxing plugin.

**Solution:**
- Process isolation (un worker per tenant? No, troppo costoso)
- User context in ogni request
- Plugin sandboxing (già nel piano)
- Input validation

### 5. **Monitoring & Observability**

**Problem:** Come monitoriamo processi C?

**Solution:**
- Metrics export (Prometheus)
- Structured logging (JSON)
- Distributed tracing (OpenTelemetry)
- Error tracking (Sentry)

```rust
// Metrics
use prometheus::{Counter, Histogram};

lazy_static! {
    static ref REQUESTS_TOTAL: Counter = Counter::new(
        "convergio_requests_total", "Total requests"
    ).unwrap();
    static ref REQUEST_DURATION: Histogram = Histogram::with_opts(
        HistogramOpts::new("convergio_request_duration", "Request duration")
    ).unwrap();
}
```

### 6. **Distribution & Updates**

**Problem:** Come distribuiamo aggiornamenti core C?

**Solution:**
- Versioning semantico
- Rolling updates (Kubernetes)
- Backward compatibility API
- Feature flags

### 7. **Cost Management**

**Problem:** Costi LLM possono esplodere.

**Solution:**
- Usage quotas per subscription tier
- Cost tracking per utente
- Alerts quando si avvicina limite
- Auto-throttling

```rust
// Cost tracking
struct UsageTracker {
    db: Database,
}

impl UsageTracker {
    async fn check_quota(&self, user_id: &str) -> Result<()> {
        let usage = self.db.get_monthly_usage(user_id).await?;
        let tier = self.db.get_subscription_tier(user_id).await?;
        
        if usage.requests >= tier.max_requests {
            return Err("Quota exceeded");
        }
        
        if usage.cost >= tier.max_cost {
            return Err("Cost limit exceeded");
        }
        
        Ok(())
    }
}
```

---

## Recommended Implementation Plan

### Phase 1: Core API Extraction (Month 1)
1. Refactor core C: rimuovere CLI, creare API pura
2. Build come shared library (.so/.dylib)
3. Test API con test C semplici

### Phase 2: Rust Wrapper (Month 2)
1. Creare Rust FFI bindings
2. Implementare CoreWorker struct
3. Test chiamate C da Rust

### Phase 3: API Gateway MVP (Month 3)
1. HTTP server (Axum/Warp)
2. Process pool manager
3. Basic auth
4. Single endpoint: `/api/v1/chat`

### Phase 4: State Externalization (Month 4)
1. Migrare memory store a PostgreSQL
2. Session state in Redis
3. Test state sharing tra processi

### Phase 5: Production Hardening (Month 5)
1. Health checks, auto-restart
2. Rate limiting, quotas
3. Monitoring, logging
4. Error handling robusto

### Phase 6: Scaling & Optimization (Month 6+)
1. Load testing
2. Performance optimization
3. Caching layer
4. Database optimization

---

## Cost Estimation

### Infrastructure (per 1000 utenti attivi)

- **API Gateway**: 3 pods × $50/mese = $150/mese
- **Database**: PostgreSQL managed (AWS RDS) = $200/mese
- **Redis**: Managed cache = $50/mese
- **Load Balancer**: AWS ALB = $25/mese
- **Monitoring**: Datadog/New Relic = $100/mese
- **Total**: ~$525/mese base

### LLM Costs (variabile)

- Assumendo 1000 richieste/utente/mese
- Media $0.02 per richiesta (Claude/GPT-4o)
- **Total**: 1M richieste × $0.02 = $20,000/mese

**Critical:** I costi LLM sono 40x l'infrastructure! Deve essere passato all'utente o limitato.

---

## Conclusion

**Recommended Approach:**
1. Core C as library + Rust API Gateway
2. Process pool per isolamento
3. State in database esterno
4. Usage-based pricing per coprire costi LLM
5. Quotas e rate limiting aggressivi

**Key Takeaways:**
- ✅ Manteniamo core C (performance, codebase)
- ✅ Scalabilità via process pool + horizontal scaling API
- ✅ Isolamento via process separation
- ⚠️ Costi LLM sono il problema principale, non infrastructure
- ⚠️ State management richiede refactoring significativo

**Next Steps:**
1. Validare approccio con POC (1 settimana)
2. Refactor core C API (1 mese)
3. Build Rust wrapper (2 settimane)
4. Test end-to-end (2 settimane)

---

## Related Documents

**Master Index:** [V7Plan-MASTER-INDEX.md](./V7Plan-MASTER-INDEX.md) - Complete documentation hub

**Single Source of Truth:**
- [V7Plan-CRITICAL-REVIEW.md](./V7Plan-CRITICAL-REVIEW.md) - Optimized unified plan ⭐

**Architecture:**
- [V7Plan.md](./V7Plan.md) - Core architecture
- [V7Plan-Enhanced.md](./V7Plan-Enhanced.md) - Web platform & telemetry
- [V7Plan-Voice-WebPlatform.md](./V7Plan-Voice-WebPlatform.md) - Voice & web stack

**Infrastructure:**
- [V7Plan-Billing-Security.md](./V7Plan-Billing-Security.md) - Billing & security
- [V7Plan-Business-Case.md](./V7Plan-Business-Case.md) - Cost analysis

