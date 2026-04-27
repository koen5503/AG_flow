import os

Import("env")

env_file = ".env"

if os.path.exists(env_file):
    print(f"Reading from {env_file}")
    with open(env_file, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            
            key, value = line.split("=", 1)
            
            # Escape quotes for C++ macros
            if value.startswith('"') and value.endswith('"'):
                inner_val = value[1:-1]
                value = f'\\"{inner_val}\\"'
                
            env.Append(CPPDEFINES=[(key, value)])
            print(f"Injected {key}={value}")
else:
    print(f"WARNING: {env_file} not found!")
