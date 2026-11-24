# 参数与命名档案

默认值集中在 [defaults.yaml](../assets/defaults.yaml)。所有参数均可省略。自然语言「只要 C++，类型加 Ku，字段 m_ 前缀，先别问」与结构化参数等价。深度合并对象，列表整体替换；只覆盖写明的键。发现未知键或冲突只指出相关项，不假装它已经生效。

| 参数 | 可选值 | 解析方式 |
| --- | --- | --- |
| `targets` | `[cpp]`、`[rust]`、`[rust, cpp]` | 只展开请求的语言。 |
| `questions` | `auto / always / never` | auto 在新的完整任务里就影响公开 API、所有权或身份的未定选择集中问一次；已知答案直接采用。always 在生成前展示未定项和推荐拼写，等用户回答；设置齐全时确认整组值。never 采用可行的假设并标出。遇到影响正确性的未知语义，给带明确条件的草稿。 |
| `naming.cpp` | `project / kuai / standalone / snake` | project 读同模块现有类和使用点，空项目回落 standalone。kuai 使用所查模块的词汇和前缀。 |
| `naming.rust` | `project / idiomatic` | 空项目回落 idiomatic。 |
| `naming.overrides` | 按语言分组的可选字段 | 可设 `type_case`、`type_prefix`、`variant_case`、`variant_prefix`、`method_case`、`field_case`、`field_prefix`；大小写值为 `PascalCase / camelCase / snake_case / SCREAMING_SNAKE_CASE`。布尔前缀与自由函数特例按现有 API。 |
| `ownership` | `auto / tree / shared / arena-id / borrowed` | auto 先看构造和消费证据，普通无可观察身份示例暂按 tree。显式共享或哈希合并的节点采用 shared / arena-id。borrowed 要能指出实际 owner 与失效条件。 |
| `cpp.kind` | `auto / enum / strong-integer` | 封闭独立 ADT 默认枚举，已注册的 kuai Object 域使用其强类型整数；开放扩展按宿主协议。 |
| `cpp.standard` | `project / 20 / 23 / 26` | 读取实际 target 配置。独立项目以 26 为首选；明确选择较低标准时生成可编译实现。 |
| `cpp.contracts` | `project / assert / cxx26` | project 复用断言封装；独立项目默认 assert。cxx26 对纯内部不变量启用经探测的 contract_assert，必需的运行时检查仍显式处理。 |
| `cpp.reflection` | `project / off / cxx26` | 宿主已有惯例则沿用。独立项目默认用 X-macro。cxx26 仅派生可反射的信息，保留业务映射表。 |

解析时先满足源语义和目标 API/ABI，再对仍可选择的部分应用「本次显式值 > 项目相邻代码 > 个人默认 > 内置回落」。例如明确指定 `field_prefix: m_` 可作用于新类型，重命名已有公开字段则需要用户请求 API 变更。选定档案后再应用具体覆盖（个人专门指定的字段可覆盖档案自带的拼写）。无法同时满足的条件原样说明。

## 让用户通过调用端选择

下面是同一个独立表达式类型的几组拼写。例中的 kuai 风格行演示新增类型的命名方式；接入 kuai 时必须先找到真实 kind 域和可用编号。

| 档案 | 类型和构造器 | 读取 kind、查询类型、读字段 |
| --- | --- | --- |
| C++ standalone | `Expr`、`AddExpr`、`ExprKind::Add` | `expr.getKind()`、`expr.as<AddExpr>()`、`m_left` |
| C++ kuai | `KuExpr`、`KuAddExpr`、`kAddExpr`（新叶子常量示意） | `expr.getKind()`、`expr.as<KuAddExpr>()`、`m_left` |
| C++ snake | `expr`、`add_expr`、`expr_kind::add` | `node.get_kind()`、`node.as<add_expr>()`、`left_` |
| Rust idiomatic | `Expr`、`Expr::Add` | `match expr`、命名字段 `source_line` 或位置字段模式 |

类型前缀只用于类型名，kind 的拼写单独决定。`field_prefix` 加在已转换大小写的字段名前；snake 的尾下划线是档案默认项，显式指定 `field_prefix` 时替换该装饰。Rust 关键字优先合法 raw identifier 或符合仓库惯例的重命名，大小写归一造成的冲突显式消歧，并保存源名对应关系。

提问示例：「这个新模块你想读到 `expr.getKind() / m_sourceLine`，还是 `expr.get_kind() / source_line_`？类型可以单独加 `Ku`。只选命名即可，Rust 继续用 `Expr::Add`。」用户已经点明的项直接代入，避免重复询问。

## 一次覆盖的完整例子

```yaml
targets: [cpp, rust]
questions: never
naming:
  cpp: standalone
  overrides:
    cpp:
      type_prefix: Ast
      field_case: snake_case
      field_prefix: m_
ownership: arena-id
cpp:
  standard: 26
  contracts: assert
```

这次输出中可以看到 `AstExpr`、`AstAddExpr`、`m_source_line`；孩子边使用强类型 `AstExprId`。下一次仍读取个人默认。用户明确说“以后新项目都用 snake”时，把 `naming.cpp` 更新为 `snake` 即可。
