# Communication Module — Observer Pattern

Agent-to-agent messaging for the **G7 – Object-Oriented Agentic AI Framework for Multi-Agent Systems** project.
Language: **C++17**, built with **CMake**, no external dependencies.

It also provides `LLMAgent`, which connects this module to the LLM module
(`LLMStrategy::generate()` on branch `feature/llm-adapter`): agents receive work
through the bus (**Observer**) and think with an interchangeable LLM (**Strategy**).

## 1. Project layout

```
communication/
├── CMakeLists.txt
├── include/communication/
│   ├── Message.h             # Immutable message + MessageType
│   ├── IObserver.h           # Observer interface
│   ├── ISubject.h            # Subject interface
│   ├── TopicMatcher.h        # Topic matching: "*", "task.*", "task.done"
│   ├── MessageBus.h          # ConcreteSubject — central "post office"
│   └── CommunicatingAgent.h  # ConcreteObserver — base class for agents
├── include/agents/LLMAgent.h # Agent = Observer + LLM Strategy
├── include/llm/LLMStrategy.h # Same interface as feature/llm-adapter (+ mocks)
├── src/                      # Implementations (.cpp)
├── demo/main.cpp             # Demo: pipeline, runtime LLM switch, error handling
├── tests/test_communication.cpp   # 23 unit tests
└── docs/observer_class_diagram.puml  # UML class diagram (PlantUML)
```

## 2. Build & run

```bash
cd communication
cmake -S . -B build
cmake --build build
./build/comm_tests      # run unit tests   (Windows: build\Debug\comm_tests.exe)
./build/comm_demo       # run the demo
```

Visual Studio: *File → Open → Folder* and select this directory; CMake is detected automatically.

## 3. How the Observer Pattern is applied

| Pattern role | Class | Responsibility |
|---|---|---|
| Subject (interface) | `ISubject` | `attach`, `detach`, `notify` |
| ConcreteSubject | `MessageBus` | Keeps observers per topic and dispatches messages |
| Observer (interface) | `IObserver` | `update(const Message&)` |
| ConcreteObserver | `CommunicatingAgent` (and subclasses) | Receives, stores and handles messages |

Flow: agent A calls `publish("task.research", ...)` → `MessageBus::notify()` finds every observer subscribed to a matching topic → calls each observer's `update()` → agent B handles it in a topic handler or in `onMessage()`.

**Two ways to communicate**
- *Topic-based publish/subscribe*: `subscribe("task.*")`, `publish("task.created", ...)` — the sender does not need to know who receives it (loose coupling).
- *Direct messages*: `send("Planner", ...)`, `reply(msg, ...)` — delivered to exactly one agent by name.

## 4. LLMAgent — combining Observer and Strategy

```cpp
auto bus = std::make_shared<MessageBus>();

// name, LLM strategy, role, input topic, output topic
auto researcher = std::make_shared<LLMAgent>("Researcher", std::make_shared<OpenAILLM>(),
                                             "You are a researcher.", "task.research", "task.write");
auto writer     = std::make_shared<LLMAgent>("Writer", std::make_shared<ClaudeLLM>(),
                                             "You are a writer.", "task.write", "task.result");
researcher->connect(bus); researcher->start();
writer->connect(bus);     writer->start();

planner->publish("task.research", "AI trends 2026", MessageType::Request);
// Researcher -> "task.write" -> Writer -> "task.result" -> Planner

researcher->setStrategy(std::make_shared<GeminiLLM>(), "Gemini");  // swap at runtime
```

| Incoming message | What LLMAgent does |
|---|---|
| `Request` on its input topic | `generate()` → publish on output topic (or reply if none) |
| Direct `Request` (`send("Writer", ...)`) | `generate()` → `reply()` to the sender |
| `Event` / `Response` | Ignored |
| LLM throws, or returns `"ERROR..."` / `"GEMINI ERROR..."` (as `GeminiLLM` does) | Publishes on `agent.error` and replies with `MessageType::Error` |
| `setStrategy()` called | Publishes `agent.strategy.changed` |

`buildPrompt()` is `virtual`, so a specialised agent can override how the prompt is built.

## 5. OOP principles

- **Abstraction**: `IObserver` and `ISubject` define *what* to do, not *how*.
- **Encapsulation**: `Message` has getters only and cannot be modified in transit; all `MessageBus` state is `private` and accessed through mutex-protected methods.
- **Inheritance**: `CommunicatingAgent` → `PlannerAgent`, `WriterAgent`, … (and the framework's `BaseAgent`).
- **Polymorphism**: the bus only knows `IObserver`, yet calls the right `update()`/`onMessage()` for each agent type.
- **Template Method**: `update()` is `final` (fixed steps: store in inbox → topic handler → `onMessage()`); subclasses override only `onMessage()`.

## 6. Design highlights

1. **`weak_ptr` in the bus**: agents hold a `shared_ptr<MessageBus>`, the bus holds only `weak_ptr<IObserver>` → no reference cycles; a destroyed agent disappears from the bus automatically (covered by a test).
2. **Fault isolation**: an agent that throws does not prevent other agents from receiving the message; the error is recorded in `getErrors()`.
3. **Dead letters**: messages nobody received are kept for debugging (`getDeadLetters()`).
4. **No duplicate delivery**: an agent subscribed to both `"*"` and `"task.*"` receives a message once; senders never receive their own messages.
5. **Thread-safe & re-entrant**: messages are delivered *outside* the lock, so an agent can reply from inside its handler without deadlock.
6. **Bounded history**: filter by topic/sender; oldest messages are dropped beyond `maxHistory`.
7. **Safe strategy swap**: `LLMAgent` copies the `shared_ptr<LLMStrategy>` before calling it, so swapping providers while a request is running is safe.

## 7. Integration with the other modules

### LLM module (branch `feature/llm-adapter`)
`include/llm/LLMStrategy.h` contains exactly the same `LLMStrategy` interface and the
`OpenAILLM` / `ClaudeLLM` mocks as `LLMstrategy.cpp`, so this module builds on its own.
To use the real `GeminiLLM` (curl + nlohmann/json):

1. Move the classes from `LLMstrategy.cpp` into a header (e.g. `LLMStrategy.h`) and move
   its `main()` into a separate demo file — otherwise linking fails with a duplicate `main`.
2. Replace `include/llm/LLMStrategy.h` with that header.
3. Link curl in CMake: `find_package(CURL REQUIRED)` and
   `target_link_libraries(communication PUBLIC CURL::libcurl)`.

No change is needed in `LLMAgent`: it only depends on `LLMStrategy::generate()`.

### Core — Agent / BaseAgent
`Agent _Observe.cpp` on `main` currently has its own small `Observer` (`onEvent`, raw
`Observer*` list). To unify, let the core agent inherit from `LLMAgent` (or
`CommunicatingAgent`) and replace `notify(msg)` with `publish(topic, msg)`:

```cpp
class Agent : public agentfw::agents::LLMAgent {
public:
    using LLMAgent::LLMAgent;
    void execute(const std::string& prompt) {
        publish("task.status.changed", name() + ": START PROCESSING...");
        std::cout << "[" << name() << "] " << ask(prompt) << "\n";
        publish("task.status.changed", name() + ": COMPLETED.");
    }
};
```

### Task lifecycle — State Pattern
When `TaskContext` changes state, publish an event so agents/monitors are notified:
```cpp
void TaskContext::setState(std::unique_ptr<TaskState> s) {
    std::string from = state_->name();
    state_ = std::move(s);
    bus_->publish(Message("TaskManager", "task.status.changed",
                          id_ + ": " + from + " -> " + state_->name()));
}
```
Any interested agent just calls `subscribe("task.status.*")`.

### Testing
`tests/test_communication.cpp` uses a tiny built-in test framework (`TEST_CASE`, `CHECK` macros). To switch to GoogleTest, change `TEST_CASE("...")` → `TEST(Communication, Name)` and `CHECK(x)` → `EXPECT_TRUE(x)`.

## 8. Suggested topic conventions

| Topic | Published by | Content |
|---|---|---|
| `task.created` | Core Engine | A new task |
| `task.status.changed` | TaskContext (State) | `"<id>: OLD -> NEW"` |
| `task.<job>` | Agent | Ask another agent to handle a job (e.g. `task.research`) |
| `agent.error` | LLMAgent / anyone | Error report |
| `agent.strategy.changed` | LLMAgent | LLM provider was swapped |
| `broadcast` | Anyone | General announcement |
| `direct` | `send()` | Private message (default) |
