import sympy

regions = 1
cities = 2
students = 4

total_cities = regions * cities
students_per_region = cities * students
total_students = regions * cities * students

grades = [sympy.symbols('g' + str(i)) for i in range(total_students)]

print(grades)

grades_num = [30, 40, 20, 80, 85, 10,
10, 20, 30, 40, 50, 60,
60, 50, 40, 30, 20, 10,
70, 55, 35, 80, 95, 27,

35, 45, 25, 85, 90, 15,
15, 25, 35, 45, 55, 65,
65, 55, 45, 35, 25, 15,
75, 60, 40, 85, 100, 32,

20, 30, 10, 70, 75, 0,
0, 10, 20, 30, 40, 50,
50, 40, 30, 20, 10, 0,
60, 45, 25, 70, 85, 17, ]

#mean_cities = [sympy.symbols('mc' + str(i)) for i in range(total_cities)]
#stddev_cities = [sympy.symbols('sc' + str(i)) for i in range(total_cities)]
#mean_regions = [sympy.symbols('mr' + str(i)) for i in range(regions)]
#stddev_regions = [sympy.symbols('sr' + str(i)) for i in range(regions)]

mean_cities = [0 for i in range(total_cities)]
stddev_cities = [0 for i in range(total_cities)]

mean_regions = [0 for i in range(regions)]
stddev_regions = [0 for i in range(regions)]

mean_country = 0
stddev_country = 0

for r in range(regions):
    for c in range(cities):
        for s in range(students):
            mean_cities[r*cities + c] = mean_cities[r*cities + c] + grades[r * students_per_region + c * students + s]
        mean_cities[r * cities + c] = mean_cities[r*cities + c] / students
        mean_regions[r] = mean_regions[r] + mean_cities[r*cities + c]
    mean_regions[r] = mean_regions[r] / cities
    mean_country = mean_country + mean_regions[r]
mean_country = mean_country / regions

for r in range(regions):
    for c in range(cities):
        for s in range(students):
            stddev_cities[r*cities + c] = stddev_cities[r*cities + c] + (grades[r * students_per_region + c * students + s] - mean_cities[r * cities + c]) ** 2
            stddev_regions[r] = stddev_regions[r] + (grades[r * students_per_region + c * students + s] - mean_regions[r]) ** 2
            stddev_country = stddev_country + (grades[r * students_per_region + c * students + s] - mean_country) ** 2
        stddev_cities[r * cities + c] = sympy.sqrt(stddev_cities[r * cities + c] / (students - 1))

    stddev_regions[r] = sympy.sqrt(stddev_regions[r] / (students_per_region - 1))

stddev_country = sympy.sqrt(stddev_country / (total_students - 1))

"""
[print("mean_cities", i, mc.simplify()) for i, mc in enumerate(mean_cities)]
print()
[print("mean_regions", i, mc.simplify()) for i, mc in enumerate(mean_regions)]
print()
print("mean_country", mean_country)
print()
[print("stddev_cities", i, mc.simplify()) for i, mc in enumerate(stddev_cities)]
print()
[print("stddev_regions", i, mc.simplify()) for i, mc in enumerate(stddev_regions)]
print()
sympy.pprint(stddev_country.simplify())
"""
subs = {}

[ subs.update({('g'+str(i)):g}) for i, g in enumerate(grades_num) ]

print()
print()
[print("mean_cities", i, mc.simplify()) for i, mc in enumerate(mean_cities)]
print()
[print("mean_regions", i, mr.simplify()) for i, mr in enumerate(mean_regions)]
print()
print("mean_country", mean_country.simplify())
print()
[print("stddev_cities", i, sc.simplify()) for i, sc in enumerate(stddev_cities)]
print()
[print("stddev_regions", i, sr.simplify()) for i, sr in enumerate(stddev_regions)]
print()
sympy.pprint(stddev_country.expand().simplify())

"""
print()
print()
[print("mean_cities", i, mc) for i, mc in enumerate(mean_cities)]
print()
[print("mean_regions", i, mr) for i, mr in enumerate(mean_regions)]
print()
print("mean_country", mean_country)
print()
[print("stddev_cities", i, sc) for i, sc in enumerate(stddev_cities)]
print()
[print("stddev_regions", i, sr) for i, sr in enumerate(stddev_regions)]
print()
sympy.pprint(stddev_country)"""