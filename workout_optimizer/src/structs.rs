#[derive(Clone)]
pub struct Exercise {
    pub name: String,
    pub primary: Vec<String>,
    pub secondary: Vec<String>,
    pub isometric: Vec<String>,
    pub is_compound: bool,
    pub is_leg: bool,
    pub critical: bool,
}

#[derive(Clone)]
pub struct MuscleGroup {
    pub target: f64,
    pub upper_bound: f64,
}

#[derive(Clone)]
pub struct RoutineEntry {
    pub exercise: String,
    pub sets: i32,
}