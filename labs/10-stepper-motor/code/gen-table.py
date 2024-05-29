#!/usr/bin/env python3

import sys

base_freqs = {
    "C": 16.35,
    "CS": 17.32,
    "D": 18.35,
    "DS": 19.45,
    "E": 20.60,
    "F": 21.83, 
    "FS": 23.12,
    "G": 24.50,
    "GS": 25.96,
    "A": 27.50,
    "AS": 29.14,
    "B": 30.87
}


def freq(note, octave):
    return base_freqs[note] * 2 ** octave


# step delay 1000 -> frequency 498
# step delay 2000 -> frequency 249
# Using 4/4 time signature and 120 BPM -> 1 half note per second
def stepper_params(note, octave, length):
    f = freq(note, octave)
    step_delay = round(49800 / f)
    n_steps = round(length / 0.5 / ((step_delay + 1) / 1000))
    return step_delay, n_steps


def print_table(notes, octaves, lengths, file=sys.stdout):
    params = [
        stepper_params(note, octave, length)
        for note, octave, length in zip(notes, octaves, lengths)
    ]
    print("uint32_t step_delays[] = {", file=file)
    for step_delay, _ in params:
        print(f"  {step_delay},", file=file)
    print("  0\n};", file=file)
    print("uint32_t step_lengths[] = {", file=file)
    for _, n_steps in params:
        print(f"  {n_steps},", file=file)
    print("  0\n};", file=file)


def print_to_file(filename, notes, octaves, lengths):
    with open(filename, "w") as infile:
        print_table(notes, octaves, lengths, file=infile)


def rick_roll():
    notes = [
        "B", "CS", "D", "D", "E", "CS", "B", "A"
    ]
    octaves = [
        2, 3, 3, 3, 3, 3, 2, 2
    ]
    lengths = [0.5] * 100
    return notes, octaves, lengths


def us_anthem():
    whole = 3
    half = whole / 2
    quarter = half / 2
    eighth = quarter / 2
    sixteenth = eighth / 2

    # From this website: https://noobnotes.net/star-spangled-banner-traditional/

    notes = [
        "G", "E", "C", "E", "G", "C",
        "E", "D", "C", "E", "FS", "G",
        "G", "G", "E", "D", "C", "B",
        "A", "B", "C", "C", "G", "E", "C",
        "E", "G", "C", "E", "G", "C",
        "E", "D", "C", "E", "FS", "G",
        "G", "G", "E", "D", "C", "B",
        "A", "B", "C", "C", "G", "E", "C",
        "E", "E", "E", "F", "G", "G",
        "F", "E", "D", "E", "F", "F",
        "F", "E", "D", "C", "B",
        "A", "B", "C", "E", "FS", "G",
        "G", "C", "C", "C", "B", "A", "A", "A", "D", "F", "E", "D", "E", "C", "B",
        "G", "G", "C", "D", "E", "FS", "G",
        "C", "E", "E", "F", "D", "C"
    ]

    octaves = [
        3, 3, 3, 3, 3, 4,
        4, 4, 4, 3, 3, 3,
        3, 3, 4, 4, 4, 3,
        3, 3, 4, 4, 3, 3, 3,
        3, 3, 3, 3, 3, 4,
        4, 4, 4, 3, 3, 3,
        3, 3, 4, 4, 4, 3,
        3, 3, 4, 4, 3, 3, 3,
        4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 3,
        3, 3, 4, 3, 3, 3,
        3, 4, 4, 4, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 3,
        3, 3, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4
    ]
    
    lengths = [
        eighth + sixteenth,
        sixteenth,
        quarter,
        quarter,
        quarter,
        half,
        eighth + sixteenth,
        sixteenth,
        quarter,
        quarter,
        quarter,
        half,
        eighth,
        eighth,
        quarter + eighth,
        eighth,
        quarter,
        half,
        eighth,
        eighth,
        quarter,
        quarter,
        quarter,
        quarter,
        quarter,
        eighth + sixteenth,
        sixteenth,
        quarter,
        quarter,
        quarter,
        half,
        eighth + sixteenth,
        sixteenth,
        quarter,
        quarter,
        quarter,
        half,
        eighth,
        eighth,
        quarter + eighth,
        eighth,
        quarter,
        half,
        eighth,
        eighth,
        quarter,
        quarter,
        quarter,
        quarter,
        quarter,
        eighth,
        eighth,
        quarter,
        quarter,
        quarter,
        half,
        eighth,
        eighth,
        quarter,
        quarter,
        quarter,
        half,
        quarter,
        quarter + eighth,
        eighth,
        quarter,
        half,
        eighth,
        eighth,
        quarter,
        quarter,
        quarter,
        half,
        quarter,
        quarter,
        quarter,
        eighth,
        eighth,
        quarter,
        quarter,
        quarter,
        eighth,
        eighth,
        eighth,
        eighth,
        eighth,
        eighth,
        quarter,
        quarter,
        eighth,
        eighth,
        quarter + eighth,
        eighth,
        eighth,
        eighth,
        half,
        eighth,
        eighth,
        quarter + eighth,
        eighth,
        quarter,
        half + quarter,
    ]
    return notes, octaves, lengths


def main():
    print_to_file("rick_roll.h", *rick_roll())
    print_to_file("us_anthem.h", *us_anthem())
    print_to_file(
        "1-freq-test.h",
        ["C", "D", "E", "F", "G", "A", "B"] * 2 + ["C"],
        [3] * 7 + [4] * 7 + [5],
        [1] * 15,
    )
    


if __name__ == "__main__":
    main()
