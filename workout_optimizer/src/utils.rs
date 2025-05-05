use std::collections::{HashMap, HashSet};
use std::fs::File;
use std::io::{Write, BufWriter};

use crate::structs::{Exercise, MuscleGroup, RoutineEntry};

// Constants
pub const TOTAL_DAYS: i32 = 6;
pub const TIME_PER_SET: f64 = 2.0;
pub const MIN_EXERCISES_PER_DAY: i32 = 3;
pub const MAX_EXERCISES_PER_DAY: i32 = 6;
pub const MIN_SETS: i32 = 2;
pub const MAX_SETS: i32 = 5;
pub const MAX_TIME_PER_DAY: f64 = 50.0;

pub fn is_muscle_recently_used(routine: &Vec<Vec<RoutineEntry>>, current_day: i32, muscle: &String, exercises: &Vec<Exercise>) -> bool {
    let muscle_recovery_days = crate::data::muscle_recovery_days();
    let recovery_days = muscle_recovery_days.get(muscle).unwrap_or(&0);
    for day in (current_day - recovery_days).max(0)..current_day {
        for entry in &routine[day as usize] {
            if let Some(ex) = exercises.iter().find(|e| e.name == entry.exercise) {
                if ex.primary.contains(muscle) || ex.secondary.contains(muscle) {
                    return true;
                }
            }
        }
    }
    false
}

pub fn compute_volumes(routine: &Vec<Vec<RoutineEntry>>, exercises: &Vec<Exercise>) -> HashMap<String, f64> {
    let mut volumes: HashMap<String, f64> = HashMap::new();
    for day in 0..TOTAL_DAYS {
        for entry in &routine[day as usize] {
            if let Some(ex) = exercises.iter().find(|e| e.name == entry.exercise) {
                for muscle in &ex.primary {
                    *volumes.entry(muscle.clone()).or_insert(0.0) += entry.sets as f64 * 1.0;
                }
                for muscle in &ex.secondary {
                    *volumes.entry(muscle.clone()).or_insert(0.0) += entry.sets as f64 * 0.5;
                }
                for muscle in &ex.isometric {
                    *volumes.entry(muscle.clone()).or_insert(0.0) += entry.sets as f64 * 0.25;
                }
            }
        }
    }
    volumes
}

pub fn compute_cost(routine: &Vec<Vec<RoutineEntry>>, mav_targets: &HashMap<String, MuscleGroup>, exercises: &Vec<Exercise>) -> f64 {
    let volumes = compute_volumes(routine, exercises);
    let mut exercise_frequency: HashMap<String, i32> = HashMap::new();
    let mut total_time_penalty = 0.0;
    let mut volume_penalty = 0.0;
    let mut frequency_penalty = 0.0;
    let mut time_variance_penalty = 0.0;
    let mut compound_first_penalty = 0.0;
    let mut inclusion_penalty = 0.0;
    let mut day_times = vec![0.0; TOTAL_DAYS as usize];
    let mut included_exercises: HashSet<String> = HashSet::new();

    // Collect all exercise names
    let all_exercises: HashSet<String> = exercises.iter().map(|e| e.name.clone()).collect();

    for day in 0..TOTAL_DAYS {
        let mut day_exercises: HashSet<String> = HashSet::new();
        let mut leg_exercises = 0;
        let mut day_time = 0.0;
        let mut compound_first = false;

        for (i, entry) in routine[day as usize].iter().enumerate() {
            if day_exercises.contains(&entry.exercise) {
                return f64::INFINITY;
            }
            day_exercises.insert(entry.exercise.clone());
            included_exercises.insert(entry.exercise.clone());
            *exercise_frequency.entry(entry.exercise.clone()).or_insert(0) += 1;

            if let Some(ex) = exercises.iter().find(|e| e.name == entry.exercise) {
                if i == 0 && ex.is_compound {
                    compound_first = true;
                }
                if ex.is_leg {
                    leg_exercises += 1;
                }
                day_time += entry.sets as f64 * TIME_PER_SET;
            }
        }

        if !compound_first && !routine[day as usize].is_empty() {
            compound_first_penalty += 50.0;
        }
        if routine[day as usize].len() < MIN_EXERCISES_PER_DAY as usize || 
           routine[day as usize].len() > MAX_EXERCISES_PER_DAY as usize || 
           leg_exercises > 2 {
            return f64::INFINITY;
        }
        if day_time > MAX_TIME_PER_DAY {
            total_time_penalty += 0.0 * (day_time - MAX_TIME_PER_DAY);
        }
        day_times[day as usize] = day_time;
    }

    // Penalize for each exercise not included in the routine
    for exercise in all_exercises {
        if !included_exercises.contains(&exercise) {
            inclusion_penalty += 50000.0;  // Penalty value can be adjusted
        }
    }

    let avg_time = day_times.iter().sum::<f64>() / TOTAL_DAYS as f64;
    for time in day_times {
        let diff = time - avg_time;
        time_variance_penalty += if diff > 0.0 { 0.0 } else { 0.0 } * diff.powi(2);
    }

    for (muscle, target) in mav_targets {
        if muscle == "Glutes" || muscle == "Lower Back" {
            continue;
        }
        let vol = volumes.get(muscle).cloned().unwrap_or(0.0);
        let deficit = target.target - vol;
        if deficit > 0.0 {
            let weight = 50.0;                    
            volume_penalty += weight * deficit.powi(2);
        } else if vol > target.upper_bound {
            volume_penalty += 25.0 * (vol - target.upper_bound).powi(2);
        }
    }

    for (_, freq) in exercise_frequency {
        if freq > 2 {
            frequency_penalty += 0.0 * (freq - 2) as f64;
        }
    }

    volume_penalty + frequency_penalty + total_time_penalty + time_variance_penalty + compound_first_penalty + inclusion_penalty
}

pub fn save_to_file(routine: &Vec<Vec<RoutineEntry>>, exercises: &Vec<Exercise>, mav_targets: &HashMap<String, MuscleGroup>, filename: &str) {
    let file = File::create(filename).expect("Unable to create file");
    let mut out = BufWriter::new(file);

    writeln!(out, "# 6-Day Workout Routine\n").unwrap();
    writeln!(out, "Each session starts with one compound exercise, followed by additional sets to target specific muscle groups.\n").unwrap();

    for day in 0..TOTAL_DAYS {
        writeln!(out, "## Day {}: {} Exercises", day + 1, routine[day as usize].len()).unwrap();
        for (i, entry) in routine[day as usize].iter().enumerate() {
            if let Some(ex) = exercises.iter().find(|e| e.name == entry.exercise) {
                let set_type = if i == 0 && ex.is_compound { "Straight Sets (Compound First)" } else { "Straight Sets" };
                let primary_str = ex.primary.join(", ");
                let secondary_str = if !ex.secondary.is_empty() { format!(", secondary: {}", ex.secondary.join(", ")) } else { String::new() };
                let isometric_str = if !ex.isometric.is_empty() { format!(", isometric: {}", ex.isometric.join(", ")) } else { String::new() };
                writeln!(out, "- **{}**: {} - {} sets of 8-12 reps *({}{}{})*", set_type, entry.exercise, entry.sets, primary_str, secondary_str, isometric_str).unwrap();
            }
        }
        let total_time = routine[day as usize].iter().map(|e| e.sets as f64 * TIME_PER_SET).sum::<f64>();
        writeln!(out, "- **Time Estimate**: {} minutes\n", total_time as i32).unwrap();
    }

    writeln!(out, "## Weekly Volume Breakdown").unwrap();
    let volumes = compute_volumes(routine, exercises);
    for (muscle, target) in mav_targets {
        let vol = volumes.get(muscle).cloned().unwrap_or(0.0);
        let status = if muscle == "Glutes" || muscle == "Lower Back" {
            "not optimized"
        } else if vol < target.target {
            "below target"
        } else if vol > target.upper_bound {
            "exceeds upper bound"
        } else {
            "on target"
        };
        writeln!(out, "- **{}**: {:.2} sets ({}–{} sets – {})", muscle, vol, target.target, target.upper_bound, status).unwrap();
    }
}