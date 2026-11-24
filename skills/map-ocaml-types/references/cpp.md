# C++ 对象、转换与标准特性

## 对接 kuai

以下路径来自 `yslib/kuai` 的 `2b883e6`，用于辨认项目约定。在不同分支工作时以当前源码为准。

| 去哪里看 | 生成代码该复用什么 |
| --- | --- |
| [KuRtti.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuRtti.h) | `KuRtti<Kind>` 持 kind。`KU_RTTI_LEAF` 精确相等，`KU_RTTI_RANGE` 判 `begin < kind && kind < end`。traits 校验 descriptor 自报的 object_type 与目标类型相同，防止继承到父类注册。`as<T>()` 校验注册、域与继承关系，保留 const，运行时不匹配给空指针。`ku_isa<T>(ptr)` 处理空指针。 |
| [KuObjectDefs.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuObjectDefs.h)、[KuObject.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuObject.h) | `KuObjectKind` 是带域的强类型整数 `KuRttiKind<KuObjectKindTag>`；sections / items 展开 kind 常量、分组界限和字符串。库内叶子、用户预留区及界限值各有用途。已有编号必须保留。`KuObject` 自身注册为 `KU_UNKNOWN` 的 leaf；叶子到根直接向上转型。 |
| [KuDeviceData.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuDeviceData.h)、[KuTensor.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuTensor.h) | `KuTensor` 在构造时把 `kTensor` 传给 `KuDeviceData`；两者分别注册 leaf / range。同一个具体对象可查询叶子和中间类。 |
| [KuPointer.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuPointer.h)、[KuRefCounted.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuRefCounted.h) | `KuObject` 同时继承 RTTI 与引用计数。实际采用侵入持有时复用 `ku_sp<T>` / `ku_make_sp<T>`；`as<T>()` 得到借用，不改变 refcount。retain、adopt、借用在跨边界时分清。 |
| [KuDebug.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/core/include/kuai/core/KuDebug.h) | `KU_ASSERT` 在 NDEBUG 下跳过求值；`KU_VERIFY` 始终求值，release 不保证检查结果；`KU_UNREACHABLE` 自身触发失败。`cast<T>()` 仅有静态继承检查，生成调用点时先用支配该路径的 kind 证明。 |
| [ku_object.h](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/src/kuai_c/include/kuai/kuai_c/ku_object.h)、[ku_object.rs](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/crates/kurt/src/ku_object.rs) | 公开 C kind 有自己的协议。例如 `KU_OBJECT_TENSOR` 为 2，内部 `kTensor` 为 1001。Rust 的安全包装和 sys 分层，读取外部 raw tag 后显式匹配。只在用户要求跨语言互操作时生成适配层。 |
| [KurtBuildOptions.cmake](https://github.com/yslib/kuai/blob/2b883e6295a9d04e672749ba0df1fd16836a6fb6/kurt-cpp/cmake/KurtBuildOptions.cmake) | 此版本的 MSVC 分支设置 C++23，其他分支设置 C++26。编译选项与实际可用特性分别确认。 |

## 独立封闭 ADT 的生成规则

1. 定义本域 `enum class ExprKind`，一个构造器对应一项。让 `Rtti<ExprKind>` 存唯一的 kind，业务根 `Expr` 和叶子不再存判别字段。普通 `record`、纯枚举和标量包装走值表示。用户请求跨模块稳定编号时再指定固定宽度、显式值和边界验证。
2. 从一张 X-macro 列表生成 kind、type-to-kind 注册、名称或 visitor 的机械分支。在宏外手写字段、不变量和业务动作。使用带域前缀的宏名，用完 `#undef`；测试增加一项时哪些编译点必须更新。分派表达式使用穷尽 switch，未知外部整数在解码处返回错误。内部不可达尾部使用项目失败路径。
3. 封闭域只允许已知叶子构造根，不让外界提供任意 kind。禁用基类公开复制/赋值以保护 kind 与动态类型的一致性、避免切片。向上转型用普通语言规则。`as<T>()` 返回可空借用，const 传播，错误类型或跨域目标在编译期拒绝。需要 `cast<T>()` 时写明其前置条件；独立实现可对内部不变量做 debug 断言。对公共输入的校验要在 release 下也执行。
4. 给泛型族 `'a expr` 单独设计域身份。例如用 `ExprKindTag<T>` 标识实例化域，或在 `Expr<T>` 上把可查询的类型限制为同一个 `T`。仅检查一个 `Lit` tag 不足以把 `Lit<int>` 解释成 `Lit<double>`。运行时选择 payload 类型时使用明确的二级 tag 或受控类型擦除。
5. RTTI 基础设施只负责分类。叶子通常 `final`。通过 `unique_ptr<Expr>` 删除时根有虚析构；arena 擦除对象时明确由谁执行非平凡析构。多态调用只在业务确实需要时添加。强类型 ID 的 arena 可以持有 `unique_ptr<Expr>` 或项目节点池，边只存 ID。仅需稳定 ID 时避免许诺引用地址稳定。
6. 保持 `const` / `[[nodiscard]]`，只给保证不抛的 API 标 `noexcept`。构造路径确立内部非空边，一般使用只读访问器；对于 `mutable` 保留源程序可观察的更新。内建算术、比较、clone 与遍历仅按目标语义生成。

## C++26 的具体落点

| 场景 | 优先实现与可验证回落 |
| --- | --- |
| 检查已证明的内部转换 | 项目有断言宏时沿用。显式选用契约且 `__cpp_contracts >= 202502L`、构建已启用该策略时，可写 `contract_assert(p != nullptr);`。只放无副作用谓词。契约有 ignore/observe 等模式；必须拒绝的外部值走显式分支。 |
| 穷举已声明的 enum 项 | 在支持 `<meta>` 和 `__cpp_impl_reflection` 的工具链，可在 `consteval` 内用 `std::meta::enumerators_of(^^ExprKind)` 校验枚举元数据。下面的最小探针无需 expansion statements。稳定 wire 名称、叶子类对应关系、号段等业务数据仍以表为准。 |
| const 与非 const 的访问器 | C++23 的显式对象形参 `this Self&` 可合并实现，并限制临时对象借用；若需要兼容 C++20，使用 `&`/`const &` 重载并删除从右值借用的重载。`std::expected` 同样属于 C++23。 |
| 选择可用实现 | 在相同 flags、标准库及编译前端（含 CUDA 经过的 host/device 路径）做小型编译探针。探针成功才编译含新语法的路径，回落保持相同 API、失败语义和 ABI。 |

可选反射探针（针对独立、封闭、无别名枚举）：

```cpp
#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202603L && __has_include(<meta>)
#include <meta>
enum class ProbeKind { Lit, Add };
static_assert(std::meta::enumerators_of(^^ProbeKind).size() == 2);
#endif
```

实际探测能力时让未满足 `#if` 的分支 `#error`，防止“跳过后编译成功”被当成特性可用。工程中的兼容头才使用回落。参阅 [契约求值](https://eel.is/c++draft/basic.contract.eval) 和 [静态反射查询](https://eel.is/c++draft/meta.reflection.member.queries)。
