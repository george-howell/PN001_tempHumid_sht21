%% SHT-21 TEMP HUMID CALC

clc; clearvars; close all;

% input data (16-bit unsigned value)
S_RH = 25424;
S_T = 3082;

% RH
RH = -6 + 125*(S_RH/2^16);

% TEMP
Temp = -46.85 + 175.72*(S_T/2^16);