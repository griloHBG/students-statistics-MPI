import numpy as np


class GradeIndex:
    def __init__(self, grade, index):
        self.grade = grade
        self.index = index


regions = 20
cities = 38
students = 9

grades = np.zeros((regions, cities, students))
cit_sum = np.zeros((regions, cities))
cit_sq_sum = np.zeros((regions, cities))

reg_sum = np.zeros((regions, 1))
reg_sq_sum = np.zeros((regions, 1))

cou_sum = 0
cou_sq_sum = 0

cit_avg = np.zeros((regions, cities))
cit_dev = np.zeros((regions, cities))
cit_min = np.zeros((regions, cities))
cit_max = np.zeros((regions, cities))
cit_med = np.zeros((regions, cities))

reg_avg = np.zeros((regions, 1))
reg_dev = np.zeros((regions, 1))
reg_min = np.zeros((regions, 1))
reg_max = np.zeros((regions, 1))
reg_med = np.zeros((regions, 1))

cou_avg = 0
cou_dev = 0
cou_min = 0
cou_max = 0
cou_med = 0


def std_dev(sq_sum:float, sum:float, avg:float, length:int) -> float:
    if((sq_sum - 2*avg*sum + length * avg * avg) < 0):
        print("shit")
    return np.sqrt((sq_sum - 2*avg*sum + length * avg * avg)/(length-1))

for r in range(regions):
    # print(r)
    for c in range(cities):
        for s in range(students):
            grades[r, c, s] = (r * cities * students + c * students + s) % 101
            cit_sum[r, c] += grades[r, c, s]
            cit_sq_sum[r, c] += grades[r, c, s] * grades[r, c, s]

        cit_avg[r, c] = cit_sum[r, c] / students
        cit_dev[r, c] = std_dev(cit_sq_sum[r, c], cit_sum[r, c], cit_avg[r, c], students)

        reg_sum[r] += cit_sum[r, c]
        reg_sq_sum[r] += cit_sq_sum[r, c]
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
                print("---------------------------------------------------------------------------------------------------------------------------------------------------------------")
        else:
            if cnp == equal:
                cnp = 0
                np += 1
                print("---------------------------------------------------------------------------------------------------------------------------------------------------------------")

        print("[{0}] [{1}]".format(np, cnp), end="")
        print("[{0}] avg = {1:.2f} dev = {2:.2f} || {3}    ||    [{4}]".format(c, cit_avg[r,c], cit_dev[r,c], grades[r,c], counter))
        counter += 1
        cnp += 1
    print()