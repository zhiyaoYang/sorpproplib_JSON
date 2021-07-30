% Minimal example demonstrating how to use Matlab interface to the
% SorpPropLib
%
% Setup: Define paths, load functions
%
clc;
clear;

path_db = '.\data\JSON\sorpproplib.json';
path_dll = '.\data\win64bit\libsorpPropLib.dll';
path_header = '.\data\win64bit\workingPair.h';

addpath('.\struct_call');
addpath('.\struct_call\refrigerants');
addpath('.\struct_call\adsorption');
addpath('.\struct_call\absorption');

% Adsorption: Working pair based on the volumetric approach
%
% Setup for working pair
%
wp_as = 'silica gel pellet';
wp_st = 'Fuji A';
wp_rf = 'Water';

wp_iso = 'Freundlich';
no_iso = 1;
rf_psat = 'VaporPressure_EoS1';
no_p_sat = 1;
rf_rhol ='SaturatedLiquidDensity_EoS1';
no_rhol = 1;

% Define some input parameters for calculations
%
p_start = 1000;             % Pa
T_fix = 353.15;             % K

p = zeros(1, 1000);
T = zeros(1, 1000);

for i = 1 : 1000
   p(i) = (i-1) + p_start;  % Pa
   T(i) = T_fix;            % K
end

% Initialize working pair object
%
refWPair = newWorkingPair(path_db, path_dll, path_header, ...
                    wp_as, wp_st, wp_rf, ...
                    wp_iso, no_iso, rf_psat, ...
                    no_p_sat, rf_rhol, no_rhol);
                
% Perform calculations:
% Results in first row are performed with fucntion that are always defined
% Results in second row are performed with functions depending on isotherm
% approach
%
p_sat = zeros(1, 1000);
dp_sat_dT = zeros(1, 1000);
T_sat = zeros(1, 1000);

w_pT = zeros(2, 1000);
p_wT = zeros(2, 1000);
T_pw = zeros(2, 1000);

dw_dp_pT = zeros(2, 1000);
dw_dT_pT = zeros(2, 1000);
dp_dw_wT = zeros(2, 1000);
dp_dT_wT = zeros(2, 1000);

for i = 1 : 1000
    % Execute refrigerant functions
    %
    p_sat(i) = ref_p_sat_T(T(i), refWPair);
    dp_sat_dT(i) = ref_dp_sat_dT_T(T(i), refWPair);
    T_sat(i) = ref_T_sat_p(p_sat(i), refWPair);
    
    % Execute functions that are always defined
    %
    w_pT(1, i) = ads_w_pT(p(i), T(i), refWPair);
    p_wT(1, i) = ads_p_wT(w_pT(1, i), T(i), refWPair);
    T_pw(1, i) = ads_T_pw(p(i), w_pT(1, i), refWPair);
    
    dw_dp_pT(1, i) = ads_dw_dp_pT(p(i), T(i), refWPair);
    dw_dT_pT(1, i) = ads_dw_dT_pT(p(i), T(i), refWPair);
    dp_dw_wT(1, i) = ads_dp_dw_wT(w_pT(1, i), T(i), refWPair);
    dp_dT_wT(1, i) = ads_dp_dT_wT(w_pT(1, i), T(i), refWPair);
    
    % Execute functions that are only defined for adsorption isotherms
    % based on the surface approach and using vapor pressure as input
    %
    w_pT(2, i) = ads_sur_w_pTpsat(p(i), T(i), p_sat(i), refWPair);
    p_wT(2, i) = ads_sur_p_wTpsat(w_pT(2, i), T(i), p_sat(i), refWPair);
    T_pw(2, i) = ads_sur_T_pwpsat(p(i), w_pT(2, i), p_sat(i), refWPair);
    
    dw_dp_pT(2, i) = ads_sur_dw_dp_pTpsat(p(i), T(i), p_sat(i), refWPair);
    dw_dT_pT(2, i) = ads_sur_dw_dT_pTpsat(p(i), T(i), p_sat(i), ...
                                          dp_sat_dT(i), refWPair);
    dp_dw_wT(2, i) = ads_sur_dp_dw_wTpsat(w_pT(2, i), T(i), p_sat(i), ...
                                          refWPair);
    dp_dT_wT(2, i) = ads_sur_dp_dT_wTpsat(w_pT(2, i), T(i), p_sat(i), ...
                                          dp_sat_dT(i), refWPair);
end

% Delete working pair object (i.e., necessary to free memory!)
%
delWorkingPair(refWPair);            
clear refWPair;
unloadlibrary libsorpPropLib;