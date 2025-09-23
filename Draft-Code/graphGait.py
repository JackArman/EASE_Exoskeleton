import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

# 1) RAW JOINT DATA
# hipRightGait is set to its original hardcoded values
hipRightGait = np.array([
    0.444408834, 0.446378444, 0.447181602, 0.446534546, 0.444555754,
    0.44167249, 0.438403041, 0.435104709, 0.431803519, 0.428186508,
    0.423750571, 0.418031449, 0.410801834, 0.402159281, 0.392475268,
    0.382249264, 0.371940883, 0.361855035, 0.352108682, 0.342670401,
    0.333435109, 0.324296727, 0.315192117, 0.306114271, 0.29710261,
    0.288223599, 0.279548379, 0.271135017, 0.263022213, 0.25523342,
    0.247781926, 0.240668297, 0.233876164, 0.227373995, 0.221124361,
    0.215093483, 0.209257371, 0.203601548, 0.19811274, 0.192762414,
    0.187491954, 0.182216295, 0.176855445, 0.171379818, 0.165841156,
    0.160361801, 0.155083568, 0.150103835, 0.145434597, 0.141005461,
    0.1367128, 0.132496006, 0.128411569, 0.124674405, 0.121651278,
    0.119807204, 0.119622431, 0.121501346, 0.125700023, 0.132290417,
    0.14116961, 0.152102437, 0.164782903, 0.178896222, 0.194161315,
    0.210338075, 0.227205111, 0.244534383, 0.262087803, 0.2796385,
    0.296995705, 0.314013691, 0.330585374, 0.346634911, 0.3621156,
    0.377003142, 0.391271264, 0.404852681, 0.417607305, 0.429325209,
    0.439774511, 0.448778056, 0.456278693, 0.462354773, 0.467167722,
    0.470865743, 0.473491195, 0.474939948, 0.474992229, 0.473409571,
    0.470065511, 0.465065706, 0.458813434, 0.451990084, 0.445444401,
    0.440014358, 0.436336164, 0.434702119, 0.435005202
])

# hipLeftGait is now the negative of hipRightGait's values
hipLeftGait = -np.array([
    0.444408834, 0.446378444, 0.447181602, 0.446534546, 0.444555754,
    0.44167249, 0.438403041, 0.435104709, 0.431803519, 0.428186508,
    0.423750571, 0.418031449, 0.410801834, 0.402159281, 0.392475268,
    0.382249264, 0.371940883, 0.361855035, 0.352108682, 0.342670401,
    0.333435109, 0.324296727, 0.315192117, 0.306114271, 0.29710261,
    0.288223599, 0.279548379, 0.271135017, 0.263022213, 0.25523342,
    0.247781926, 0.240668297, 0.233876164, 0.227373995, 0.221124361,
    0.215093483, 0.209257371, 0.203601548, 0.19811274, 0.192762414,
    0.187491954, 0.182216295, 0.176855445, 0.171379818, 0.165841156,
    0.160361801, 0.155083568, 0.150103835, 0.145434597, 0.141005461,
    0.1367128, 0.132496006, 0.128411569, 0.124674405, 0.121651278,
    0.119807204, 0.119622431, 0.121501346, 0.125700023, 0.132290417,
    0.14116961, 0.152102437, 0.164782903, 0.178896222, 0.194161315,
    0.210338075, 0.227205111, 0.244534383, 0.262087803, 0.2796385,
    0.296995705, 0.314013691, 0.330585374, 0.346634911, 0.3621156,
    0.377003142, 0.391271264, 0.404852681, 0.417607305, 0.429325209,
    0.439774511, 0.448778056, 0.456278693, 0.462354773, 0.467167722,
    0.470865743, 0.473491195, 0.474939948, 0.474992229, 0.473409571,
    0.470065511, 0.465065706, 0.458813434, 0.451990084, 0.445444401,
    0.440014358, 0.436336164, 0.434702119, 0.435005202
])

kneeLeftGait = -np.array([
    0.293922677, 0.308415849, 0.323563743, 0.338261082, 0.351931327,
    0.364414184, 0.37576485, 0.38604935, 0.395212261, 0.40305695,
    0.409316254, 0.413756137, 0.416258744, 0.416855181, 0.415709541,
    0.413076997, 0.409267058, 0.404624974, 0.399515454, 0.394285656,
    0.389207288, 0.384423983, 0.379934661, 0.375625241, 0.371339165,
    0.366959236, 0.362468149, 0.357965953, 0.353642807, 0.349720093,
    0.346382432, 0.343725231, 0.341733759, 0.340295787, 0.339235638,
    0.338353037, 0.33745033, 0.336344883, 0.334875719, 0.33292067,
    0.330426597, 0.327441818, 0.324133795, 0.320780042, 0.317728318,
    0.315333481, 0.313889379, 0.313579808, 0.314466543, 0.316516612,
    0.319655563, 0.323831542, 0.329076222, 0.335548785, 0.343545278,
    0.353467718, 0.36576555, 0.380873062, 0.399154765, 0.420858093,
    0.446061921, 0.474619878, 0.506109066, 0.539829581, 0.574900073,
    0.610442321, 0.645747011, 0.680306384, 0.713692708, 0.745382196,
    0.774653349, 0.800631175, 0.822443481, 0.839398729, 0.851103849,
    0.857487461, 0.858735368, 0.855178714, 0.847183364, 0.83508245,
    0.816614714, 0.794538212, 0.769194496, 0.740914169, 0.710018537,
    0.676836816, 0.641719979, 0.605031643, 0.56711349, 0.528256892,
    0.488737301, 0.448964013, 0.409728228, 0.372412603, 0.338966899,
    0.311556523, 0.292022651, 0.281398498, 0.27963726
])

kneeRightGait = np.array([
    0.303732502, 0.318647517, 0.333480779, 0.346822298, 0.358243285,
    0.368154717, 0.377387544, 0.386690556, 0.396356197, 0.406109777,
    0.415257546, 0.422983922, 0.42864897, 0.431979369, 0.433100446,
    0.432431158, 0.430501166, 0.427777877, 0.4245635, 0.420982838,
    0.417035979, 0.412675942, 0.407875381, 0.402664439, 0.397134494,
    0.391419379, 0.385671325, 0.380043586, 0.374677203, 0.369678617,
    0.365087016, 0.360850586, 0.356843717, 0.352921151, 0.348975502,
    0.344965656, 0.340920427, 0.336935835, 0.333174712, 0.329855254,
    0.327207246, 0.325398426, 0.324470127, 0.324333704, 0.324836253,
    0.325850747, 0.327329746, 0.32930561, 0.331861743, 0.335112395,
    0.339206226, 0.344341628, 0.350777205, 0.358827953, 0.368848235,
    0.381203877, 0.396240881, 0.414250028, 0.435427411, 0.45983105,
    0.487344589, 0.517658649, 0.550276421, 0.584544419, 0.619705335,
    0.654963198, 0.689533962, 0.722664235, 0.75363382, 0.781771712,
    0.806494078, 0.827341331, 0.843991926, 0.856246508, 0.863995527,
    0.867188291, 0.86581687, 0.859921496, 0.84961116, 0.83508245,
    0.816614714, 0.794538212, 0.769194496, 0.740914169, 0.710018537,
    0.676836816, 0.641719979, 0.605031643, 0.56711349, 0.528256892,
    0.488737301, 0.448964013, 0.409728228, 0.372412603, 0.338966899,
    0.311556523, 0.292022651, 0.281398498, 0.27963726
])

# 2) NORMALIZE AND EXTEND DATA TO 3 GAIT CYCLES

def smooth_normalize(arr, n_points=200):
    """
    Smooths and normalizes a given array to a specified number of points.
    It also removes the initial value and the linear trend from start to end.
    """
    x_old = np.linspace(0, 1, len(arr))
    x_new = np.linspace(0, 1, n_points)
    resampled = np.interp(x_new, x_old, arr)
    resampled -= resampled[0] # Set the first point to zero
    # Remove linear trend from start to end
    resampled -= np.linspace(0, resampled[-1], n_points)
    return resampled

N = 200 # Number of points per gait cycle
num_cycles = 3 # Number of gait cycles to plot
x_len = N * num_cycles # Total number of points for the plot
phase_percent = np.linspace(0, 100 * num_cycles, x_len) # X-axis for plotting (Gait Cycle %)

# Normalize and phase shift the data
hipL_smooth = smooth_normalize(hipLeftGait, N)
kneeL_smooth = smooth_normalize(kneeLeftGait, N)
hipR_smooth = smooth_normalize(hipRightGait, N)
kneeR_smooth = smooth_normalize(kneeRightGait, N)

# Apply a phase shift to the right leg data to simulate alternating gait
# This shifts the second half of the cycle to the beginning
hipR_smooth = np.concatenate((hipR_smooth[N//2:], hipR_smooth[:N//2]))
kneeR_smooth = np.concatenate((kneeR_smooth[N//2:], kneeR_smooth[:N//2]))

# Repeat the smoothed data for the specified number of gait cycles
hipL_cycle = np.tile(hipL_smooth, num_cycles)
kneeL_cycle = np.tile(kneeL_smooth, num_cycles)
hipR_cycle = np.tile(hipR_smooth, num_cycles)
kneeR_cycle = np.tile(kneeR_smooth, num_cycles)

# 3) PLOT

plt.figure(figsize=(16, 9)) # Set the figure size for better visualization
ax = plt.gca() # Get the current axes

# Define subphases of gait for background shading
gait_subphases = [
    {"name": "Initial Contact (IC)", "start": 0, "end": 2, "color": "#CCE5FF"}, # Light Blue
    {"name": "Loading Response (LR)", "start": 2, "end": 12, "color": "#B3D9FF"}, # Medium Blue
    {"name": "Mid Stance (MSt)", "start": 12, "end": 31, "color": "#99CCFF"}, # Blue
    {"name": "Terminal Stance (TSt)", "start": 31, "end": 50, "color": "#80BFFF"}, # Darker Blue
    {"name": "Pre-Swing (PSw)", "start": 50, "end": 62, "color": "#FFCCB3"}, # Light Orange
    {"name": "Initial Swing (ISw)", "start": 62, "end": 75, "color": "#FFB380"}, # Medium Orange
    {"name": "Mid Swing (MSw)", "start": 75, "end": 87, "color": "#FF994D"}, # Orange
    {"name": "Terminal Swing (TSw)", "start": 87, "end": 100, "color": "#FF8000"} # Dark Orange
]

# Plot background subphases for each gait cycle
phase_patches = [] # To store legend patches for subphases
for cycle in range(num_cycles):
    offset = cycle * 100 # Offset for each subsequent cycle
    for phase in gait_subphases:
        start = phase["start"] + offset
        end = phase["end"] + offset
        ax.axvspan(start, end, color=phase["color"], alpha=0.5) # Add shaded region
        if cycle == 0: # Only add patches for the first cycle to the legend
            phase_patches.append(Patch(facecolor=phase["color"], label=phase["name"] + " (Left Leg)"))

# Plot joint angles for hip and knee, left and right
plt.plot(phase_percent, hipL_cycle, label="Hip Left", color='blue', linewidth=2)
plt.plot(phase_percent, hipR_cycle, label="Hip Right (Phase Shifted)", color='red', linewidth=2)
plt.plot(phase_percent, kneeL_cycle, label="Knee Left", color='green', linewidth=2)
plt.plot(phase_percent, kneeR_cycle, label="Knee Right (Phase Shifted)", color='orange', linewidth=2)

# Add a horizontal line at y=0 for reference
plt.axhline(0, color='black', linestyle='--', alpha=0.5)

# Define gait event markers
gait_event_lines = [
    ("Initial Contact (IC)", 0),
    ("Loading Response Ends", 2),
    ("Mid Stance Ends", 12),
    ("Terminal Stance Ends", 31),
    ("Toe Off (TO)", 50),
    ("Initial Swing Ends", 62),
    ("Mid Swing Ends", 75),
    ("Terminal Swing Ends", 87)
]

# Plot vertical lines and text labels for gait events
y_text_pos = plt.ylim()[1] * 0.95 # Position for text labels
for cycle in range(num_cycles):
    offset = cycle * 100
    for label, percent in gait_event_lines:
        x = percent + offset
        plt.axvline(x, color='gray', linestyle=':', linewidth=1.5, alpha=0.7) # Vertical line
        if cycle == 0: # Only add text labels for the first cycle
            plt.text(x + 1, y_text_pos, label, rotation=90, verticalalignment='top', fontsize=8, color='dimgray')

# Set plot labels and title
plt.xlabel("Gait Cycle (%)")
plt.ylabel("Joint Angle (radians)")
plt.title("Smoothed Gait Data Over 3 Gait Cycles with Subphases (Left Leg Reference)")
plt.grid(True) # Add grid

# Create a custom legend combining plot lines and phase patches
handles, labels = ax.get_legend_handles_labels()
# Filter out handles/labels that are not the main plot lines
plot_handles = [h for h, l in zip(handles, labels) if l in ["Hip Left", "Hip Right (Phase Shifted)", "Knee Left", "Knee Right (Phase Shifted)"]]
plot_labels = ["Hip Left", "Hip Right (Phase Shifted)", "Knee Left", "Knee Right (Phase Shifted)"]

# Combine plot handles/labels with phase patches for the final legend
plt.legend(handles=plot_handles + phase_patches, labels=plot_labels + [p["name"] for p in gait_subphases],
           loc='upper left', bbox_to_anchor=(1.02, 1), ncol=1, fontsize=9) # Position legend outside plot

plt.tight_layout(rect=[0, 0, 0.78, 1]) # Adjust layout to prevent legend overlap
plt.savefig("gait_3_cycles.png") # Save the plot as a PNG image
plt.show()