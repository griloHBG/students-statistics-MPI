import subprocess
import numpy as np

test_median_exe = "../cmake-build-debug/test_median/test_median"
studentsseq_exe = "../cmake-build-debug/studentsseq/studentsseq"

regions =       [11, 50, 100]
cities =        [10, 51, 100]
students =      [10, 51]
seed =          10
repetitions =   5

test_median_times = np.zeros((len(regions), len(cities), len(students)))
studentsseq_times = np.zeros((len(regions), len(cities), len(students)))

subprocess.run(['cmake', '--build', '/media/grilo/Data/Git/students-statistics-MPI/cmake-build-debug', '--target', 'all', '--', '-j', '6'])

for ri, r in enumerate(regions):
    for ci, c in enumerate(cities):
        for si, s in enumerate(students):
            print("Running {3}x with regions = {0}, cities = {1}, students = {2}".format(r, c, s, repetitions))
            for rep in range(repetitions):
                test_median = subprocess.Popen([test_median_exe, str(r), str(c), str(s), str(seed)], stdout=subprocess.PIPE, universal_newlines=True)
                studentsseq = subprocess.Popen([studentsseq_exe, str(r), str(c), str(s), str(seed)], stdout=subprocess.PIPE, universal_newlines=True)

                try:
                    test_median_output = float(str(test_median.communicate()[0]))
                    studentsseq_output = float(str(studentsseq.communicate()[0]))
                except:
                    pass

                test_median_times[ri, ci, si] += test_median_output
                studentsseq_times[ri, ci, si] += studentsseq_output

                print(rep)

            test_median_times[ri, ci, si] /= repetitions
            studentsseq_times[ri, ci, si] /= repetitions

            print(test_median_times[ri, ci, si], studentsseq_times[ri, ci, si])

with open('test_median_times.dat', 'a') as f:
    for ri, r in enumerate(regions):
        for ci, c in enumerate(cities):
            f.write(" ".join(str(a) for a in test_median_times[ri, ci]))
        f.write('\n')

with open('studentsseq_times.dat', 'a') as f:
    for ri, r in enumerate(regions):
        for ci, c in enumerate(cities):
            f.write(" ".join(str(a) for a in studentsseq_times[ri, ci]))
        f.write('\n')
