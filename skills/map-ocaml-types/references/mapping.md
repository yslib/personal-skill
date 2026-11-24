# OCaml 语义与目标表示

| OCaml 形状 | Rust | C++ | 生成前关心的语义 |
| --- | --- | --- | --- |
| `type name = other` | `type Name = Other` | `using Name = Other` | 类型缩写可互换；若使用处要求强身份，显式提出 newtype 的 API 变化。 |
| `type id = Id of int32` | 单构造器可用 `struct Id(i32)` | `struct Id` 封装 `std::int32_t`，构造显式 | 外层名称引入新的名义类型。若用户要保持构造器 API，保留 `Id` 的构造入口。 |
| `type mode = Read \| Write` | 无载荷 `enum Mode` | `enum class Mode` | 只有显式的 wire/ABI 协议才固定导出的数值。 |
| `type t = Nil \| Add of expr * expr` | `enum T { Nil, Add(Expr, Expr) }`，需要时在边上间接化 | 基类 `T`，单个 kind，`Nil` / `Add` 叶子 | 逐项匹配；含载荷 sum 的空分支也能作为一个值构造。 |
| `A of int32 * bool` 与 `B of (int32 * bool)` | `A(i32, bool)` 与 `B((i32, bool))` | `A(int32_t, bool)` 与 `explicit B(std::pair<int32_t, bool>)` | 前者是二元构造器，后者是一元且载荷是 pair。类型别名指向 pair 时同样保留该分组。 |
| `type point = { x : float; y : float }`，`P of { x : float; y : float }` | 独立 `struct Point`、具名字段分支 `P { x, y }` | 普通结构体、持有相应字段的叶子 | 保留名字，原来只在构造器内部可见的载荷，默认留在分支内部。 |
| `'a t`、`('a, 'b) t` | `T<A>`、`T<A, B>` | `template<class A, class B>` 等 | 在使用操作的地方加约束；同一 RTTI 域里同一 tag 不能识别多个 payload 实例化（见 C++ 参考）。 |
| `'a option`、`('a,'e) result` | `Option<A>`、`Result<A,E>` | `std::optional<A>`、支持时 `std::expected<A,E>` 或项目 Result | 区分 `None` 与 `Some None`；只有空指针确实只代表一层 None 时才合并编码。 |
| `'a list`、`'a array` | 顺序读写场景可用 `Vec<A>`；参数可借 `&[A]` | 拥有序列可用 `std::vector<A>`；参数可借 `std::span<const A>` | list 是持久链表；若依赖常数时间 cons/tail、共享尾部或身份，保留共享链。array 有共享可变语义，固定长度时限制 resize。 |
| `type a = … and b = …` | 同模块命名，直接循环边用 `Box`、共享句柄或 ID 打断 | 先声明类型，按完成类型的需求定义，必要时在类型完整处定义析构 | 按值存储图的每个环都要有间接边。`Vec<T>`/向量已提供间接存储；`Option<T>`/optional 自身通常需要知道 T 大小。 |
| `type 'a wrapper = { mutable value : 'a }`、`'a ref` | 唯一 owner 用 `&mut`；观察到共享更新时按线程模型选 `Rc<RefCell<_>>` 等 | 唯一 owner 提供受控 mutator；必须共享时引用同一存储 | 不同路径读到同一次写入属于源语义。只读 payload 对外返回只读借用。 |
| `type t`、`private` 声明 | 模块内私有字段/构造入口，导出受控 API | 不完整类型、私有构造函数或工厂 | 遵守 `.mli`，保留可观察操作。不要从抽象名猜测布局。 |

### 标量和借用

- `int32/int64` 对应 `i32/i64`、`std::int32_t/std::int64_t`。OCaml `int` 采用带标记的机器整数语义，32/64 位宿主通常有 31/63 个有效位（含符号）。普通模型先看业务范围；需要运算与溢出精确一致时指定源目标字宽和显式 wrap。Rust `isize`、C++ `intptr_t` 的范围不同；C++ 使用无符号运算实现所需的模语义。
- `bool/float` 通常对应 `bool/f64` 和 `bool/double`。OCaml `char` 是一个字节，对应 `u8/std::uint8_t`；字符串可包含任意字节。文本契约保证 UTF-8 时用 Rust `String`；否则 `Vec<u8>`。C++ `std::string` 可持有内嵌 NUL，借用时保留长度。`unit` 在返回位置用 `()/void`，存储位置使用 `()` 或空结构。
- `Box` / `unique_ptr` 适合独占树。Rust 对递归字段提供间接层，C++ `unique_ptr<Base>` 默认删除器要求可通过基类正确析构。独占拥有的非空孩子必须在构造时建立非空不变量。函数接受 view 时说明背后 owner；保存 view 时检查 reallocation / mutation 与生命周期。
- 需要 DAG、共享常量或 CSE 时，采用 `Rc` / `Arc`、宿主侵入句柄或 `ExprId` + 节点表。稳定 ID 与稳定地址是两个契约。区分 `IndexedMap<ExprId, Node>`（节点存储）与 `vector<ExprId>`（边列表）；可删除并复用槽位时按需使用 generation。若 ID 来自多个 arena 或多个泛型实例，明确由类型品牌、实例标识或封装的构造路径保证其来源。append-only DAG 可以只接受指向同一 arena 已有节点的边，由此维持无环性。仅当现有调用依赖结构相等或比较时生成对应操作，身份和指针比较另行定义。
- Rust 默认 `match &expr` 或直接 match 一个借用。保持递归边恰好一层所需间接；把用例一并编译，包括测试所需的 `Debug`（例如 `assert_eq!`）。按使用点决定 `Clone / Copy / Eq / Hash` 和 trait bound，包含 `f64` 的树不自动派生 `Eq`，也不为本地 enum 添加 `repr(C)`。

### 需要单独处理的源形状

对 GADT 的返回类型索引和等式约束、存在类型、开放多态变体的行约束、`type t = ..` 的扩展机制，写清目标需保留哪些静态保证。确知封闭的多态变体可收敛为本地 enum；开放的保持已知/未知扩展和相应分派。遇到一等函数要保留捕获、逃逸和调用约定。上下文不足时指出具体声明、拟采用的表示及被推迟的保证，其余普通声明照常生成。语法以 [OCaml 手册](https://ocaml.org/manual/5.3/typedecl.html) 为准。
