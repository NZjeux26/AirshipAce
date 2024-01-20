import math

# Constants
gravity_on_earth = 3.71  # gravitational acceleration on Mars in m/s^2
air_density_sea_level = 0.988  # density of air at Martain 'sea level' in kg/m^3
standard_pressure_sea_level = 95025  # standard sea level pressure in Pascals (1013.25 Millibars)
gas_constant = 287.05  # specific gas constant for dry air in J/(kg·K)

# Airship specifications
length = 232.5  # length of the airship in meters
diameter = 30.5  # diameter of the airship in meters
radius = diameter / 2  # radius of the airship

# Functions
def calculate_volume(length, radius):
    return math.pi * radius**2 * length

def calculate_buoyancy_force(volume_displaced, density_air, gravity):
    return volume_displaced * density_air * gravity

def calculate_pressure_at_altitude(altitude):
    return standard_pressure_sea_level * ((1 - 0.0065 * altitude / 288.15) ** 5.255)

def calculate_density_at_altitude(altitude, pressure):
    return pressure / (gas_constant * 288.15 * (1 + 0.001 * altitude / 288.15)) 

# User input (converting mass to kg)
mass_to_lift = float(input("Enter the mass to be lifted in metric tons: ")) * 1000

# Calculate the volume of the airship
volume_displaced = calculate_volume(length, radius)

# Initialize variables for the loop
altitude_increment = 100
altitude = 0

# Print table header
print("\nAltitude (m)\tMax Mass (kg)\tAcceleration with current mass(m/s^2)")

# Loop through altitudes up to 600m with 100m increments
while altitude <= 1000:
    # Calculate the pressure and density at the current altitude
    pressure = calculate_pressure_at_altitude(altitude)
    density_air = calculate_density_at_altitude(altitude, pressure)

    # Calculate the buoyancy force at the current altitude
    buoyancy_force = calculate_buoyancy_force(volume_displaced, density_air, gravity_on_earth)

    # Calculate the mass that can be lifted at the current altitude
    lifted_mass = buoyancy_force / gravity_on_earth

    # Calculate the acceleration at the current altitude (corrected)
    acceleration = buoyancy_force / mass_to_lift - gravity_on_earth

    # Print results
    print(f"{altitude}\t\t{lifted_mass:.2f}\t\t{acceleration:.2f}")

    # Increment altitude
    altitude += altitude_increment
