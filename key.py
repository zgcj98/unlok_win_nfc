# generate_password.py

# ===============================================
# 请在这里输入你的密码和密钥
# 确保密钥的长度大于或等于你的密码
# ===============================================
password = "mycomputerpassword"  # <<-- 替换为你的真实密码
key = "mysecretkey123456789"     # <<-- 替换为你的密钥

# 检查密钥长度
if len(key) < len(password):
    print("错误：密钥的长度必须大于或等于密码的长度！")
    exit()

# 进行 XOR 加密
encrypted_bytes = []
for i in range(len(password)):
    encrypted_bytes.append(ord(password[i]) ^ ord(key[i]))

# 将加密后的字节转换为 Arduino 代码所需的格式
# 使用 \xHH 格式表示十六进制值
hex_string = "".join([f"\\x{b:02X}" for b in encrypted_bytes])

print("------------------------------------")
print("已加密的密码（十六进制字符串）：")
print(hex_string)
print("------------------------------------")

print("\n请将上面的十六进制字符串（包括 \\x）复制到你的 Arduino 代码中。")
