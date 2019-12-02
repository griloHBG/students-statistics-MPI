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

regions, cities, students = 5, 9, 13

regions, cities, students = 3, 4, 6

print("-------------------------------")
print("reg =", regions, "cit =", cities, "stu =", students)
print("-------------------------------")

#grades = np.zeros((regions, cities, students))
grades = np.asarray(example_matrix).reshape((regions, cities, students))
cit_sum = np.zeros((regions, cities))
cit_sq_sum = np.zeros((regions, cities))


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


def std_dev(sq_sum:float, sum:float, avg:float, length:int) -> float:
    if(sq_sum - 2*avg*sum + length * avg * avg) < 0:
        print("shit")
    return np.sqrt((sq_sum - 2*avg*sum + length * avg * avg)/(length-1))


for r in range(regions):
    for c in range(cities):
            #grades[r, c, s] = regions*cities*students - (r * cities * students + c * students + s)# % 101
        cit_sum[r, c] = np.sum(grades[r, c])
        cit_sq_sum[r, c] = np.sum(np.square(grades[r, c]))

        cit_avg[r, c] = np.mean(grades[r, c])
        cit_dev[r, c] = np.std(grades[r, c])
        # print(c, grades[r, c])

    reg_avg[r] = reg_sum[r] / (cities * students)
    reg_dev[r] = std_dev(reg_sq_sum[r], reg_sum[r], reg_avg[r], cities * students)

    cou_sum += reg_sum[r]
    cou_sq_sum += reg_sq_sum[r]

cou_avg = cou_sum / (regions * cities * students)
cou_dev = std_dev(cou_sq_sum, cou_sum, cou_avg, regions * cities * students)

nodes = 3
np = 0
cnp = 0

equal = regions * cities // nodes
remaining = regions * cities % nodes

counter = 0

for r in range(regions):
    print(r)
    for c in range(cities):
        if np < remaining:
            if cnp == (equal + 1):
                cnp = 0
                np += 1
                print("-----------------------------------------------------------------------------------------------------------")
        else:
            if cnp == equal:
                cnp = 0
                np += 1
                print("-----------------------------------------------------------------------------------------------------------")

        print("[{0}] [{1:2}]".format(np, cnp), end="")
        print("[{0}] avg = {1:6.2f} dev = {2:6.2f} sqs = {3} || {4}    ||    [{5:2}]".format(c, cit_avg[r,c], cit_dev[r,c], cit_sq_sum[r,c] ,''.join(['{0:4d}'.format(int(g)) for g in grades[r,c]]), counter))
        counter += 1
        cnp += 1
    print()

print()
print()

for r in range(regions):
    print("[{0}] avg = {1:6.2f} dev = {2:6.2f} sqs = {3} || {4}    || ".format(r, reg_avg[r], reg_dev[r], reg_sq_sum[r],-1))
