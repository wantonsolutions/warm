import matplotlib.pyplot as plt
langs = ['CNS Bencmark', 'QP mapping', 'CNS -> WRITE']
students = [2480328,2548567,2831370.72]

students = [a/1000000.0 for a in students]

plt.ylabel("MOPS")
plt.bar(langs,students)
plt.show()
#plt.tight_layout()
plt.savefig("cns_swap.pdf")



# Single Key CNS-> to write
# 2633700.30
# 2801869.24
# 2827593.79
# 2828245.13
# 2831370.72