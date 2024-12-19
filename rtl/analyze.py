def analyze_writes(filename):
    writes_by_address = {}
    
    with open(filename, 'r') as file:
        for line in file:
            if "WRITE" in line:
                parts = line.strip().split()
                address = parts[6].split('=')[1]
                data = parts[7].split('=')[1]
                flag = parts[8] if len(parts) > 7 else ''
                time = parts[1]
                master = parts[5]
                
                if address not in writes_by_address:
                    writes_by_address[address] = []
                writes_by_address[address].append((time, data, flag, master))
    
    # Print results sorted by address
    for addr in sorted(writes_by_address.keys()):
        print(f"\nAddress {addr}:")
        for time, data, flag, master in writes_by_address[addr]:
            print(f"  Time:{time} Data: {data} Flag: {flag} Master: {master}")

if __name__ == "__main__":
    analyze_writes("a.txt")