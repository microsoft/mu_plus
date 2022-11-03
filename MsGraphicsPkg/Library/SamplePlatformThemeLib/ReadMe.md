# Sample Platform Theme Library

## About

The Sample Platform Theme Library is used to specify the fonts and display size information to
the Simple Window Manager.

## How to choose scale and font sizes

The starting point is a 3000 W x 2000 H display as a 100% size.

Start with your display size.

Scale =  (The width in pixels of the your display) / 3000.

Change the #define SCALE value to your whole number scale value.

Using Scale, multiply each font size in the sample by your Scale value and select a font point
close to your Scale value..
Keep in mind that each font must be a unique fonts.
The same font file cannot be used for multiple fonts.

## Copyright

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
