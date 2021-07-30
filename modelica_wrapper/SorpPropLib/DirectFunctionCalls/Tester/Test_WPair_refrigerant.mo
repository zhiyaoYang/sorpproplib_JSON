within SorpPropLib.DirectFunctionCalls.Tester;
model Test_WPair_refrigerant
  "Test model that creates an external adsorption working pair object calls its
   refrigerant functions"
  extends Modelica.Icons.Example;

  //
  // Definition of parameters
  //
  parameter String path_db=
    "D:/Engelpracht/04-C/sorproplib/sorpproplib_JSON/modelica_wrapper/SorpPropLib/Resources/Data/sorpproplib.json"
    "Path to database, i.e., JSON-file of database."
    annotation (Dialog(tab="General", group = "Working pair"));

  parameter String name_sorbent = "zeolite pellet"
    "Name of sorbent, e.g., 'zeolite pellet'"
    annotation (Dialog(tab="General", group = "Working pair"));
  parameter String name_sorbent_sub_type = "13X"
    "Sub-type of sorbent, e.g., '13X'"
    annotation (Dialog(tab="General", group = "Working pair"));
  parameter String name_refrigerant = "Water"
    "Name of refrigerant, e.g., 'Water'"
    annotation (Dialog(tab="General", group = "Working pair"));

  parameter String func_isotherm = "Toth"
    "Name of functional approach for isotherm, e.g., 'Toth'"
    annotation (Dialog(tab="General", group = "Working pair"));
  parameter Integer func_isotherm_ID = 1
    "ID of functional approach for isotherm, e.g., '1'"
    annotation (Dialog(tab="General", group = "Working pair"));

  parameter String func_vapor_pressure = "VaporPressure_EoS1"
    "Name of functional approach for vapor pressure, e.g., 'VaporPressure_EoS1'"
    annotation (Dialog(tab="General", group = "Working pair"));
  parameter Integer func_vapor_pressure_ID = 1
    "ID of functional approach for vapor pressure, e.g., '1'"
    annotation (Dialog(tab="General", group = "Working pair"));

  parameter String func_saturated_liquid_density = "SaturatedLiquidDensity_EoS1"
    "Name of functional approach for saturated liquid density, e.g. 'SaturatedLiquidDensity_EoS1'"
    annotation (Dialog(tab="General", group = "Working pair"));
  parameter Integer func_saturated_liquid_density_ID = 1
    "ID of functional approach for saturated liquid density, e.g., '1'"
    annotation (Dialog(tab="General", group = "Working pair"));

  //
  // Definition of inputs
  //
  Modelica.SIunits.Temperature T_sat = 303.15 + 1*time
    "Saturation temperature";

  //
  // Definition of variables for refrigerant functions
  //
  Modelica.SIunits.Pressure p_sat
    "Vapor pressure";
  Modelica.SIunits.Density d_liq
    "Saturated liquid density";

  Modelica.SIunits.Temperature T_sat_inv
    "Saturation temperature";

  Real dp_dT_sat(unit = "Pa/K")
    "Derivative of pressure wrt. temperature";
  Real drho_liq_dT_sat(unit = "kg/(m3.K)")
    "Derivative of saturated liquid density wrt. temperature";

equation
  //
  // Call external functions to calculate properties of refrigerant
  //
  p_sat = Refrigerant.p_sat_T(
      path_db,
      name_sorbent,
      name_sorbent_sub_type,
      name_refrigerant,
      func_isotherm,
      func_isotherm_ID,
      func_vapor_pressure,
      func_vapor_pressure_ID,
      func_saturated_liquid_density,
      func_saturated_liquid_density_ID,
      T_sat)
    "Calculate vapor pressure";
  d_liq = Refrigerant.rho_liq_T(
      path_db,
      name_sorbent,
      name_sorbent_sub_type,
      name_refrigerant,
      func_isotherm,
      func_isotherm_ID,
      func_vapor_pressure,
      func_vapor_pressure_ID,
      func_saturated_liquid_density,
      func_saturated_liquid_density_ID,
      T_sat)
    "Calculate saturated liquid density";


  T_sat_inv = Refrigerant.T_sat_p(
      path_db,
      name_sorbent,
      name_sorbent_sub_type,
      name_refrigerant,
      func_isotherm,
      func_isotherm_ID,
      func_vapor_pressure,
      func_vapor_pressure_ID,
      func_saturated_liquid_density,
      func_saturated_liquid_density_ID,
      p_sat) "Calculate saturation temperature";

  dp_dT_sat = Refrigerant.dp_dT_sat_T(
      path_db,
      name_sorbent,
      name_sorbent_sub_type,
      name_refrigerant,
      func_isotherm,
      func_isotherm_ID,
      func_vapor_pressure,
      func_vapor_pressure_ID,
      func_saturated_liquid_density,
      func_saturated_liquid_density_ID,
      T_sat) "Calculate derivative of pressure wrt. temperature";
  drho_liq_dT_sat = Refrigerant.drho_liq_dT_sat_T(
      path_db,
      name_sorbent,
      name_sorbent_sub_type,
      name_refrigerant,
      func_isotherm,
      func_isotherm_ID,
      func_vapor_pressure,
      func_vapor_pressure_ID,
      func_saturated_liquid_density,
      func_saturated_liquid_density_ID,
      T_sat)
    "Calculate derivative of saturated liquid density wrt. temperature";

  //
  // Annotations
  //
  annotation (Documentation(revisions="<html>
<ul>
  <li>
  July 13, 2021, by Mirko Engelpracht:<br/>
  First implementation.
  </li>
</ul>
</html>"), experiment(
      StopTime=100,
      Interval=1,
      __Dymola_Algorithm="Dassl"));
end Test_WPair_refrigerant;