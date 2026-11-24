pub enum Expr {
    Unit,
    Lit(i32),
    Add(Box<Expr>, Box<Expr>),
    Located { line: i32, inner: Box<Expr> },
}

impl Expr {
    pub fn node_count(&self) -> usize {
        match self {
            Self::Unit | Self::Lit(_) => 1,
            Self::Add(left, right) => 1 + left.node_count() + right.node_count(),
            Self::Located { inner, .. } => 1 + inner.node_count(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Expr;

    #[test]
    fn counts_the_same_tree_as_the_ocaml_example() {
        let expr = Expr::Located {
            line: 12,
            inner: Box::new(Expr::Add(Box::new(Expr::Lit(7)), Box::new(Expr::Unit))),
        };
        assert_eq!(expr.node_count(), 4);
    }
}
