use std::error::Error;

use plotters::{
    backend::BitMapBackend,
    chart::ChartBuilder,
    drawing::IntoDrawingArea,
    element::Circle,
    style::{full_palette::LIGHTBLUE, ShapeStyle, BLACK, BLUE, GREEN, RED, WHITE},
};

use crate::structs::PlayerLog;

fn calc_min_max(log: &PlayerLog) -> (f64, f64, f64, f64) {
    let x_min = log
        .real
        .iter()
        .map(|point| point.position.0 as f64)
        .fold(f64::INFINITY, f64::min);
    let x_max = log
        .real
        .iter()
        .map(|point| point.position.0 as f64)
        .fold(f64::NEG_INFINITY, f64::max);
    let y_min = log
        .real
        .iter()
        .map(|point| point.position.1 as f64)
        .fold(f64::INFINITY, f64::min);
    let y_max = log
        .real
        .iter()
        .map(|point| point.position.1 as f64)
        .fold(f64::NEG_INFINITY, f64::max);

    (x_min, x_max, y_min, y_max)
}

pub fn draw(log: &PlayerLog) -> Result<(), Box<dyn Error>> {
    // calculate all the proportions
    let (x_min, x_max, y_min, y_max) = calc_min_max(log);

    let root_w = (x_max - x_min) * 4.0;
    let mut root_h = (y_max - y_min) * 4.0;
    root_h = root_h.max(100.0);

    let gbase_x = x_min;
    let gbase_y = y_min - 15.0;

    println!("chart proportions: xmin: {x_min}, xmax: {x_max}, ymin: {y_min}, ymax: {y_max}");
    println!("root size: width: {root_w}, height: {root_h}");
    println!("base for the graph: {gbase_x}, {gbase_y}");

    let root = BitMapBackend::new("scatter_plot.png", (root_w as u32, root_h as u32)).into_drawing_area();
    root.fill(&WHITE)?;

    let mut chart = ChartBuilder::on(&root)
        .margin(10)
        .x_label_area_size(40)
        .y_label_area_size(40)
        .build_cartesian_2d(x_min..x_max, gbase_y..(y_max + 15.0))?;

    chart.configure_mesh().draw()?;

    // draw the skipped frames (red)
    chart.draw_series(log.lerp_skipped.iter().map(|data| {
        Circle::new(
            (data.position.0 as f64, data.position.1 as f64),
            2,
            Into::<ShapeStyle>::into(RED).filled(),
        )
    }))?;

    // draw real frames (green)
    chart.draw_series(log.real.iter().map(|data| {
        Circle::new(
            (data.position.0 as f64, data.position.1 as f64),
            3,
            Into::<ShapeStyle>::into(GREEN).filled(),
        )
    }))?;

    // draw the interpolated frames (black)
    chart.draw_series(log.lerped.iter().map(|data| {
        // println!("creating circle: {},{}", data.position.0, data.position.1);
        Circle::new(
            (data.position.0 as f64, data.position.1 as f64),
            1,
            Into::<ShapeStyle>::into(BLACK).filled(),
        )
    }))?;

    // draw extrapolated frames (mistake = dark blue, extrapolated = light blue)
    chart.draw_series(log.real_extrapolated.iter().map(|data| {
        Circle::new(
            (data.0.position.0 as f64, data.0.position.1 as f64),
            2,
            Into::<ShapeStyle>::into(BLUE).filled(),
        )
    }))?;

    chart.draw_series(log.real_extrapolated.iter().map(|data| {
        Circle::new(
            (data.1.position.0 as f64, data.1.position.1 as f64),
            2,
            Into::<ShapeStyle>::into(LIGHTBLUE).filled(),
        )
    }))?;

    Ok(())
}
