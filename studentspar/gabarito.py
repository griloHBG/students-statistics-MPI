import numpy as np


class GradeIndex:
    def __init__(self, grade, index):
        self.grade = grade
        self.index = index

example_matrix = [30,  40,  20,  80,  85,  10,
                  10,  20,  30,  40,  50,  60,
                  60,  50,  40,  30,  20,  10,
                  70,  55,  35,  80,  95,  27,

                  35,  45,  25,  85,  90,  15,
                  15,  25,  35,  45,  55,  65,
                  65,  55,  45,  35,  25,  15,
                  75,  60,  40,  85, 100,  32,

                  20,  30,  10,  70,  75,   0,
                  0,  10,  20,  30,  40,  50,
                  50,  40,  30,  20,  10,   0,
                  60,  45,  25,  70,  85,  17]

#regions, cities, students = 3, 4, 6
#grades = np.asarray(example_matrix).reshape((regions, cities, students))

regions, cities, students = 5, 9, 13

nodes = 5
nprocess = 0
cnp = 0

print("-------------------------------")
print("reg =", regions, "cit =", cities, "stu =", students)
print("-------------------------------")

grades = np.zeros((regions, cities, students))
print(grades.shape)

cit_avg = np.zeros((regions, cities))
cit_dev = np.zeros((regions, cities))
cit_min = np.zeros((regions, cities))
cit_max = np.zeros((regions, cities))
cit_med = np.zeros((regions, cities))
cit_sum = np.zeros((regions, cities))
cit_sq_sum = np.zeros((regions, cities))

reg_avg = np.zeros(regions)
reg_dev = np.zeros(regions)
reg_min = np.zeros(regions)
reg_max = np.zeros(regions)
reg_med = np.zeros(regions)
reg_sum = np.zeros(regions)
reg_sq_sum = np.zeros(regions)

cou_avg = 0
cou_dev = 0
cou_min = 0
cou_max = 0
cou_med = 0
cou_sum = 0
cou_sq_sum = 0

for r in range(regions):
    for c in range(cities):
        grades[r, c] = [(regions*cities*students - (r * cities * students + c * students + s)) % 101 for s in range(students)]
        cit_min[r, c] = np.min(grades[r, c])
        cit_max[r, c] = np.max(grades[r, c])
        cit_med[r, c] = np.median(grades[r, c])
        cit_avg[r, c] = np.mean(grades[r, c])
        cit_dev[r, c] = np.std(grades[r, c], ddof=1)

        cit_sum[r, c] = np.sum(grades[r, c])
        cit_sq_sum[r, c] = np.sum(np.square(grades[r, c]))

    reg_min[r] = np.min(grades[r])
    reg_max[r] = np.max(grades[r])
    reg_med[r] = np.median(grades[r])
    reg_avg[r] = np.mean(grades[r])
    reg_dev[r] = np.std(grades[r], ddof=1)

    reg_sum[r] = np.sum(grades[r])
    reg_sq_sum[r] = np.sum(np.square(grades[r]))

cou_min = np.min(grades)
cou_max = np.max(grades)
cou_med = np.median(grades)
cou_avg = np.mean(grades)
cou_dev = np.std(grades, ddof=1)

cou_sum = np.sum(grades)
cou_sq_sum = np.sum(np.square(grades))

equal = regions * cities // nodes
remaining = regions * cities % nodes

counter = 0


for r in range(regions):
    print(r)
    for c in range(cities):
        if nprocess < remaining:
            if cnp == (equal + 1):
                cnp = 0
                nprocess += 1
                print("-----------------------------------------------------------------------------------------------------------")
        else:
            if cnp == equal:
                cnp = 0
                nprocess += 1
                print("-----------------------------------------------------------------------------------------------------------")

        #print("[{0}] [{1:2}]".format(nprocess, cnp), end="")
        #print("[{0}] avg = {1:6.2f} dev = {2:6.2f} sqs = {3:8} || {4}    ||    [{5:2}]".format(c, cit_avg[r,c], cit_dev[r,c], cit_sq_sum[r,c] ,''.join(['{0:4d}'.format(int(g)) for g in grades[r,c]]), counter))
        print("Reg {} - Cid {}: menor: {}, maior: {}, mediana: {:.2f}, média: {:.2f} e DP: {:.2f}".format(r, c,
                cit_min[r,c], cit_max[r,c], cit_med[r,c], cit_avg[r,c], cit_dev[r,c]))
        counter += 1
        cnp += 1
    print()

print()
print()

for r in range(regions):
    print("[{0}] avg = {1:6.2f} dev = {2:6.2f} sqs = {3} || {4}    || ".format(r, reg_avg[r], reg_dev[r], reg_sq_sum[r],-1))
    print("Reg {}: menor: {}, maior: {}, mediana: {:.2f}, média: {:.2f} e DP: {:.2f}".format(r,
                reg_min[r], reg_max[r], reg_med[r], reg_avg[r], reg_dev[r]))

print()
print()
print("[{0}] avg = {1:6.2f} dev = {2:6.2f} sqs = {3} || {4}    || ".format(r, cou_avg, cou_dev, cou_sq_sum,-1))
print("Brasil: menor: {}, maior: {}, mediana: {:.2f}, média: {:.2f} e DP: {:.2f}".format(
    cou_min, cou_max, cou_med, cou_avg, cou_dev))
