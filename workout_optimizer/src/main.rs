use std::collections::HashMap;
use rand::thread_rng;

use crate::routines::optimize_routine;
use crate::utils::{save_to_file, TOTAL_DAYS};
use crate::structs::{Exercise, MuscleGroup, RoutineEntry};
use crate::data::{exercises, mav_targets};

mod routines;
mod utils;
mod structs;
mod data;

fn main() {
    let exercises: Vec<Exercise> = exercises();
    let mav_targets: HashMap<String, MuscleGroup> = mav_targets();
    
    let mut rng = thread_rng();
    let routine: Vec<Vec<RoutineEntry>> = optimize_routine(&exercises, &mav_targets, &mut rng);
    
    save_to_file(&routine, &exercises, &mav_targets, "routine_output.txt");
    
    println!("Routine optimized and saved for {} days.", TOTAL_DAYS);
}