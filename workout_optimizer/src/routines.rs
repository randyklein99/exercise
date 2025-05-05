use std::collections::{HashSet, HashMap};

use rand::Rng;
use rand::distributions::{Uniform, Distribution};

use crate::structs::{Exercise, MuscleGroup, RoutineEntry};
use crate::utils::{compute_cost, TOTAL_DAYS, MIN_SETS, MAX_SETS, MIN_EXERCISES_PER_DAY, MAX_EXERCISES_PER_DAY, is_muscle_recently_used, compute_volumes};

pub fn initialize_routine(exercises: &Vec<Exercise>, rng: &mut impl Rng) -> Vec<Vec<RoutineEntry>> {
    let mut routine: Vec<Vec<RoutineEntry>> = vec![Vec::new(); TOTAL_DAYS as usize];
    let compounds: Vec<&Exercise> = exercises.iter().filter(|e| e.is_compound).collect();
    let isolations: Vec<&Exercise> = exercises.iter().filter(|e| !e.is_compound).collect();
    let critical_isolations: Vec<&Exercise> = isolations.iter().filter(|e| e.critical).cloned().collect();
    let sets_dist = Uniform::new_inclusive(MIN_SETS, MAX_SETS);
    let isolation_dist = Uniform::new_inclusive(3, 5);

    for day in 0..TOTAL_DAYS {
        let mut used_exercises: HashSet<String> = HashSet::new();
        let mut leg_count = 0;

        for ex in &compounds {
            if used_exercises.insert(ex.name.clone()) && (!ex.is_leg || leg_count < 2) {
                let valid = ex.primary.iter().all(|muscle| !is_muscle_recently_used(&routine, day, muscle, exercises));
                if valid {
                    routine[day as usize].push(RoutineEntry { exercise: ex.name.clone(), sets: sets_dist.sample(rng) });
                    if ex.is_leg { leg_count += 1; }
                    break;
                }
            }
        }

        for ex in &critical_isolations {
            if used_exercises.insert(ex.name.clone()) && (!ex.is_leg || leg_count < 2) {
                let valid = ex.primary.iter().all(|muscle| !is_muscle_recently_used(&routine, day, muscle, exercises));
                if valid {
                    routine[day as usize].push(RoutineEntry { exercise: ex.name.clone(), sets: sets_dist.sample(rng) });
                    if ex.is_leg { leg_count += 1; }
                }
            }
        }

        let target_exercises = isolation_dist.sample(rng);
        for ex in &isolations {
            if used_exercises.len() >= target_exercises as usize {
                break;
            }
            if used_exercises.insert(ex.name.clone()) && (!ex.is_leg || leg_count < 2) {
                let valid = ex.primary.iter().all(|muscle| !is_muscle_recently_used(&routine, day, muscle, exercises));
                if valid {
                    routine[day as usize].push(RoutineEntry { exercise: ex.name.clone(), sets: sets_dist.sample(rng) });
                    if ex.is_leg { leg_count += 1; }
                }
            }
        }
    }
    routine
}

pub fn perturb_routine(routine: &mut Vec<Vec<RoutineEntry>>, exercises: &Vec<Exercise>, mav_targets: &HashMap<String, MuscleGroup>, rng: &mut impl Rng) {
    let day_dist = Uniform::new(0, TOTAL_DAYS);
    let action_dist = Uniform::new(0, 7);  // Updated from 6 to 7 to include new action
    let sets_dist = Uniform::new_inclusive(MIN_SETS, MAX_SETS);
    let volumes = compute_volumes(&routine, exercises);
    let under_target_muscles: HashSet<String> = mav_targets.iter()
        .filter(|(muscle, target)| volumes.get(*muscle).unwrap_or(&0.0) < &target.target && muscle != &"Glutes" && muscle != &"Lower Back")
        .map(|(muscle, _)| muscle.clone())
        .collect();

    // Collect all exercise names and currently included exercises
    let all_exercises: HashSet<String> = exercises.iter().map(|e| e.name.clone()).collect();
    let included_exercises: HashSet<String> = routine.iter().flat_map(|day| day.iter().map(|e| e.exercise.clone())).collect();
    let missing_exercises: Vec<String> = all_exercises.difference(&included_exercises).cloned().collect();

    match action_dist.sample(rng) {
        0 => {
            let day = day_dist.sample(rng);
            if !routine[day as usize].is_empty() {
                let idx = Uniform::new(0, routine[day as usize].len()).sample(rng);
                routine[day as usize][idx].sets = sets_dist.sample(rng);
            }
        },
        1 => {
            let day = day_dist.sample(rng);
            let day_routine_len = routine[day as usize].len();
            if day_routine_len > 0 {
                let idx = Uniform::new(0, day_routine_len).sample(rng);
                routine[day as usize].remove(idx);
            }
        },
        2 => {
            let day = day_dist.sample(rng);
            if routine[day as usize].len() < MAX_EXERCISES_PER_DAY as usize {
                let current_exercises: HashSet<String> = routine[day as usize].iter().map(|e| e.exercise.clone()).collect();
                let leg_count = routine[day as usize].iter().filter(|e| exercises.iter().find(|ex| ex.name == e.exercise).map_or(false, |ex| ex.is_leg)).count();
                let mut candidates: Vec<Exercise> = Vec::new();

                for ex in exercises {
                    if !current_exercises.contains(&ex.name) && (!ex.is_leg || leg_count < 2) && !ex.is_compound {
                        let valid = ex.primary.iter().all(|muscle| !is_muscle_recently_used(routine, day, muscle, exercises));
                        if valid && ex.primary.iter().any(|m| under_target_muscles.contains(m)) {
                            candidates.push(ex.clone());
                        }
                    }
                }

                if !candidates.is_empty() {
                    let ex_dist = Uniform::new(0, candidates.len());
                    let new_ex = candidates[ex_dist.sample(rng)].name.clone();
                    routine[day as usize].push(RoutineEntry { exercise: new_ex, sets: sets_dist.sample(rng) });
                }
            }
        },
        3 => {
            let day = day_dist.sample(rng);
            if !routine[day as usize].is_empty() {
                let idx = Uniform::new(0, routine[day as usize].len()).sample(rng);
                let current_exercises: HashSet<String> = routine[day as usize].iter().map(|e| e.exercise.clone()).collect();
                let leg_count = routine[day as usize].iter().filter(|e| exercises.iter().find(|ex| ex.name == e.exercise).map_or(false, |ex| ex.is_leg)).count();
                let mut candidates: Vec<Exercise> = Vec::new();

                for ex in exercises {
                    if !current_exercises.contains(&ex.name) && (!ex.is_leg || leg_count < 2) {
                        let valid = ex.primary.iter().all(|muscle| !is_muscle_recently_used(routine, day, muscle, exercises));
                        if valid && ex.primary.iter().any(|m| under_target_muscles.contains(m)) {
                            candidates.push(ex.clone());
                        }
                    }
                }

                if !candidates.is_empty() {
                    let ex_dist = Uniform::new(0, candidates.len());
                    let new_ex = candidates[ex_dist.sample(rng)].name.clone();
                    routine[day as usize][idx] = RoutineEntry { exercise: new_ex, sets: sets_dist.sample(rng) };
                }
            }
        },
        4 => {
            let day1 = day_dist.sample(rng);
            let day2 = day_dist.sample(rng);
            if !routine[day1 as usize].is_empty() && !routine[day2 as usize].is_empty() {
                let idx1 = Uniform::new(0, routine[day1 as usize].len()).sample(rng);
                let idx2 = Uniform::new(0, routine[day2 as usize].len()).sample(rng);
                let temp = routine[day1 as usize][idx1].clone();
                routine[day1 as usize][idx1] = routine[day2 as usize][idx2].clone();
                routine[day2 as usize][idx2] = temp;
            }
        },
        5 => {
            let day = day_dist.sample(rng);
            let day_routine_len = routine[day as usize].len();
            if day_routine_len > MIN_EXERCISES_PER_DAY as usize {
                let idx = Uniform::new(0, day_routine_len).sample(rng);
                routine[day as usize].remove(idx);
            }
        },
        6 => {
            // New action: Add a missing exercise to a random day
            if !missing_exercises.is_empty() {
                let day = day_dist.sample(rng);
                if routine[day as usize].len() < MAX_EXERCISES_PER_DAY as usize {
                    let new_ex = missing_exercises[rng.gen_range(0..missing_exercises.len())].clone();
                    let ex = exercises.iter().find(|e| e.name == new_ex).unwrap();
                    let valid = ex.primary.iter().all(|muscle| !is_muscle_recently_used(routine, day, muscle, exercises));
                    if valid {
                        routine[day as usize].push(RoutineEntry { exercise: new_ex, sets: sets_dist.sample(rng) });
                    }
                }
            }
        },
        _ => unreachable!(),
    }
}

pub fn optimize_routine(exercises: &Vec<Exercise>, mav_targets: &HashMap<String, MuscleGroup>, rng: &mut impl Rng) -> Vec<Vec<RoutineEntry>> {
    let mut routine = initialize_routine(exercises, rng);
    let mut best_routine = routine.clone();
    let mut current_cost = compute_cost(&routine, mav_targets, exercises);
    let mut best_cost = current_cost;

    let initial_temp = 1000000.0;
    let cooling_rate = 0.9999;
    let num_iterations = 75000;

    let mut temp = initial_temp;
    for iter in 0..num_iterations {
        let mut new_routine = routine.clone();
        perturb_routine(&mut new_routine, exercises, mav_targets, rng);
        let new_cost = compute_cost(&new_routine, mav_targets, exercises);

        if new_cost.is_finite() && (new_cost < current_cost || rng.r#gen::<f64>() < f64::exp((current_cost - new_cost) / temp)) {
            routine = new_routine;
            current_cost = new_cost;
            if new_cost < best_cost {
                best_cost = new_cost;
                best_routine = routine.clone();
                println!("New best cost at iteration {}: {}", iter, best_cost);
            }
        }
        temp *= cooling_rate;
    }

    best_routine
}