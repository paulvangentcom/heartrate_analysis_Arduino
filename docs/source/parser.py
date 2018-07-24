import matplotlib.pyplot as plt

if __name__ == '__main__':
    peaks = []
    rr = []
    raw = []
    movavg = []

    with open('HRDATA.CSV', 'r') as f:
        data = f.read().splitlines()

    for line in data:
        #print(line)
        if line.startswith('P:'):
            l = line.split(':')[-1].split(',')
            peaks.append(int(l[0]))
            rr.append(int(l[1]))
        else:
            l = line.split(',')
            raw.append(int(l[0]))
            movavg.append(int(l[1]))

    plt.plot(raw)
    plt.plot(movavg)
    plt.scatter(peaks, [raw[x] for x in peaks], color='green', s=60)
    plt.show()  
